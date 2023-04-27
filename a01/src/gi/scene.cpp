#include "scene.h"
#include "ray.h"
#include "distribution.h"
#include "light.h"
#include "material.h"
#include "mesh.h"
#include "timer.h"
#include "color.h"

#include <cfloat>
#include <iostream>

#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

Scene::Scene(RTCDevice& device)
    : scene(rtcNewScene(device)), device(device), bb_min(glm::vec3(FLT_MAX)), bb_max(glm::vec3(FLT_MIN)), center(glm::vec3(0.f)), radius(FLT_MIN) {
    // possible scene flags:
    // RTC_SCENE_FLAG_NONE, RTC_SCENE_FLAG_DYNAMIC, RTC_SCENE_FLAG_COMPACT
    // RTC_SCENE_FLAG_ROBUST, RTC_SCENE_FLAG_CONTEXT_FILTER_FUNCTION
    rtcSetSceneFlags(scene, RTC_SCENE_FLAG_NONE);
    // possible scene qualities:
    // RTC_BUILD_QUALITY_LOW, RTC_BUILD_QUALITY_MEDIUM, RTC_BUILD_QUALITY_HIGH
    rtcSetSceneBuildQuality(scene, RTC_BUILD_QUALITY_HIGH);
}

Scene::~Scene() {
    clear();
    rtcReleaseScene(scene);
    importer.FreeScene();
}

void Scene::clear() {
    // clear this scene
    mesh_files.clear();
    meshes.clear();
    rtcCommitScene(scene);
    materials.clear();
    lights.clear();
    sky.reset();
    light_distribution.reset();
    bb_min = glm::vec3(FLT_MAX), bb_max = glm::vec3(FLT_MIN), center = glm::vec3(0);
    radius = FLT_MIN;
}

void Scene::load_mesh(const std::filesystem::path& path) {
    const std::filesystem::path resolved_path = std::filesystem::exists(path) ? path : std::filesystem::path(GI_DATA_DIR) / path;
    std::cout << "loading: " << path << " (" << resolved_path << ")..." << std::endl;

    const uint32_t ass_flags = aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_OptimizeMeshes;
    const aiScene* scene_ai = importer.ReadFile(resolved_path.string().c_str(), ass_flags);
    if (!scene_ai)
        throw std::runtime_error("Error: failed to load mesh: " + path.string());

    // remember relative path
    mesh_files.push_back(path);

    // extract materials
    uint32_t material_offset = materials.size();
    materials.resize(materials.size() + scene_ai->mNumMaterials);
    for (uint32_t i = 0; i < scene_ai->mNumMaterials; ++i)
        materials[material_offset + i] = std::make_shared<Material>(scene_ai->mMaterials[i], resolved_path.parent_path());

    // extract meshes
    uint32_t mesh_offset = meshes.size();
    meshes.resize(meshes.size() + scene_ai->mNumMeshes);
    for (uint32_t i = 0; i < scene_ai->mNumMeshes; ++i) {
        const aiMesh* ai_mesh = scene_ai->mMeshes[i];
        const auto& mat = materials[material_offset + ai_mesh->mMaterialIndex];
        meshes[mesh_offset + i] = std::make_shared<Mesh>(device, scene, mat, ai_mesh);
        // update AABB and radius
        bb_min = min(bb_min, meshes[mesh_offset + i]->bb_min);
        bb_max = max(bb_max, meshes[mesh_offset + i]->bb_max);
        center = (bb_min + bb_max) * .5f;
        radius = length(bb_max - bb_min) * .5f;
    }
}

void Scene::load_sky(const std::filesystem::path& path, float intensity) {
    const std::filesystem::path resolved_path = std::filesystem::exists(path) ? path : std::filesystem::path(GI_DATA_DIR) / path;
    sky.reset(new SkyLight(resolved_path.string(), *this, intensity));
    sky->commit();
}

void Scene::add(const par_shapes_mesh* par_mesh, const std::shared_ptr<Material>& mat) {
    meshes.push_back(std::make_shared<Mesh>(device, scene, mat, par_mesh));
    // update AABB and radius
    bb_min = min(bb_min, meshes[meshes.size() - 1]->bb_min);
    bb_max = max(bb_max, meshes[meshes.size() - 1]->bb_max);
    center = (bb_min + bb_max) * .5f;
    radius = length(bb_max - bb_min) * .5f;
}

void Scene::commit() {
    // let embree build the BVH
    rtcCommitScene(scene);
    // (re-)select light sources from emissive meshes
    lights.clear();
    for (auto& mesh : meshes)
        if (mesh->is_light())
            lights.push_back(mesh->area_light.get());
    if (sky) lights.push_back(sky.get());
    // build distribution for light source importance sampling
    if (!lights.empty()) {
        std::vector<float> f(lights.size());
        for (uint32_t i = 0; i < lights.size(); ++i)
            f[i] = luma(lights[i]->power());
        light_distribution.reset(new Distribution1D(f.data(), f.size()));
    }
}

const SurfaceInteraction Scene::intersect(Ray &ray) const {
    {
        STAT("intersect");
        // init context
        RTCIntersectContext context;
        rtcInitIntersectContext(&context);
        // traverse bvh and return hit
        rtcIntersect1(scene, &context, toRTCRayHit(ray));
    }
    if (ray)
        return SurfaceInteraction(ray, get_mesh(ray.geomID));
    else
        return SurfaceInteraction(sky.get());
}

void Scene::intersect(std::vector<Ray> &rays, std::vector<SurfaceInteraction>& hits, bool coherent) const {
    {
        STAT("intersect");
        // init context
        RTCIntersectContext context;
        rtcInitIntersectContext(&context);
        // optimize for coherent or incoherent traversal?
        if (coherent)
            context.flags = RTC_INTERSECT_CONTEXT_FLAG_COHERENT;
        // traverse bvh and return hit
        rtcIntersect1M(scene, &context, (RTCRayHit*) rays.data(), rays.size(), sizeof(Ray));
    }
    for (auto& ray : rays) {
        if (ray)
            hits.emplace_back(ray, get_mesh(ray.geomID));
        else
            hits.emplace_back(sky.get());
    }
}

bool Scene::occluded(Ray &ray) const {
    {
        STAT("occluded");
        // init context
        RTCIntersectContext context;
        rtcInitIntersectContext(&context);
        // traverse bvh and return occlusion
        rtcOccluded1(scene, &context, toRTCRay(ray));
    }
    return ray.tfar < 0.f;
}

void Scene::occluded(std::vector<Ray> &rays, std::vector<bool>& hits, bool coherent) const {
    {
        STAT("occluded");
        // init context
        RTCIntersectContext context;
        rtcInitIntersectContext(&context);
        // optimize for coherent or incoherent traversal?
        if (coherent)
            context.flags = RTC_INTERSECT_CONTEXT_FLAG_COHERENT;
        // traverse bvh and return hit
        rtcOccluded1M(scene, &context, (RTCRay*) rays.data(), rays.size(), sizeof(Ray));
    }
    for (auto& ray : rays)
        hits.emplace_back(ray.tfar < 0.f);
}

std::tuple<const Light*, float> Scene::sample_light_source(float sample) const {
    assert(light_distribution && !lights.empty());
    // select and return light source
    const auto [index, pdf] = light_distribution->sample_index(sample);
    assert(index >= 0 && index < lights.size());
    return { lights[index], pdf };
}

float Scene::light_source_pdf(const Light* light) const {
    assert(light && light_distribution && light_distribution->integral() > 0.f && !lights.empty());
    return luma(light->power()) / light_distribution->integral();
}

float Scene::total_light_source_power() const {
    return light_distribution ? light_distribution->integral() : 0.f;
}

Mesh* Scene::get_mesh(uint32_t geomID) const {
    assert(geomID != RTC_INVALID_GEOMETRY_ID);
    return (Mesh*) rtcGetGeometryUserData(rtcGetGeometry(scene, geomID));
}

inline std::string fix_data_path(std::string path) {
    // remove GI_DATA_DIR from path to avoid absolute paths
    if (path.find(GI_DATA_DIR) != std::string::npos)
        path = path.substr(path.find(GI_DATA_DIR) + std::strlen(GI_DATA_DIR) + 1);
    return path;
}

json11::Json Scene::to_json() const {
    std::vector<json11::Json> mats;
    for (auto& mat : materials)
        mats.push_back(mat->to_json());
    std::vector<std::string> fixed_mesh_files;
    for (auto& path : mesh_files)
        fixed_mesh_files.push_back(fix_data_path(path.string()));
    return json11::Json::object{
        { "mesh_files", json11::Json(fixed_mesh_files) },
        { "sky", (sky ? sky->to_json() : json11::Json()) },
        { "materials", json11::Json(mats) }
    };
}

void Scene::from_json(const json11::Json &cfg) {
    if (cfg.is_object()) {
        // clear current scene
        clear();
        // load specified meshes
        if (cfg["mesh_files"].is_array())
            for (auto &file : cfg["mesh_files"].array_items())
                load_mesh(file.string_value());
        // patch materials
        if (cfg["materials"].is_array()) {
            for (auto& mat_json : cfg["materials"].array_items())
                if (mat_json["name"].is_string())
                    for (auto& mat_ptr : Material::instances)
                        if (mat_ptr->name == mat_json["name"].string_value())
                            mat_ptr->from_json(mat_json);
        }
        // load sky light
        if (cfg["sky"].is_object()) {
            sky.reset(new SkyLight);
            sky->from_json(cfg["sky"]);
        }
    }
}
