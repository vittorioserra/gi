#include "light.h"
#include "ray.h"
#include "scene.h"
#include "texture.h"
#include "buffer.h"
#include "mesh.h"
#include "material.h"
#include "sampling.h"
#include "distribution.h"
#include "timer.h"
#include "color.h"
#include <iostream>

// ------------------------------------------------
// Mesh area light

AreaLight::AreaLight(const Mesh& mesh) : mesh(mesh) {}

std::tuple<glm::vec3, Ray, float> AreaLight::sample_Li(const SurfaceInteraction& hit, const glm::vec2& sample) const {
    assert(sample.x >= 0 && sample.x < 1); assert(sample.y >= 0 && sample.y < 1);
    STAT("sampleLi");
    // sample area light source (triangle mesh)
    const auto [light, sample_pdf] = mesh.sample(sample);
    // TODO ASSIGNMENT1
    // compute the irradiance that arrives at <hit> (shading point) from <light> (point on light source)
    // additionally setup a shadow ray between the two surfaces
    // hint: see surface.h for relevant members of the SurfaceInteraction class
    // hint: you may simply ignore the sample_pdf variable for now

    glm::vec3 omega_i = glm::normalize(hit.P - light.P);
    glm::vec3 n = hit.N;

    float dot_prod = -glm::dot((omega_i), n); //-omega_i

    if(dot_prod < 0.0f){

        dot_prod = 0.0f;

    }

    float area_light = light.area;

    glm::vec3 k_e = power();
    float r = glm::length(hit.P - light.P);

    glm::vec3 L_i = k_e * (area_light*dot_prod)/(r*r);


    const glm::vec3 Le = L_i;
    Ray shadow_ray = Ray(hit.P, -omega_i, r);

    return { Le, shadow_ray, sample_pdf };
}

float AreaLight::pdf_Li(const SurfaceInteraction& light, const Ray& ray) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

std::tuple<glm::vec3, Ray, glm::vec3, float, float> AreaLight::sample_Le(const glm::vec2& sample_pos, const glm::vec2& sample_dir) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

std::tuple<float, float> AreaLight::pdf_Le(const SurfaceInteraction& light, const glm::vec3& dir) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

glm::vec3 AreaLight::power() const {
    return glm::vec3(mesh.mat->emissive_strength * mesh.surface_area() * PI);
}

// ------------------------------------------------
// Sky light

SkyLight::SkyLight() {}

SkyLight::SkyLight(const std::filesystem::path& path, const Scene& scene, float intensity) {
    load(path, scene.center, scene.radius, intensity);
    commit();
}

void SkyLight::load(const std::filesystem::path& path, const glm::vec3& scene_center, float scene_radius, float intensity) {
    // init variables
    const std::filesystem::path resolved_path = std::filesystem::exists(path) ? path : std::filesystem::path(GI_DATA_DIR) / path;
    std::cout << "loading: " << path << " (" << resolved_path << ")..." << std::endl;
    tex = std::make_shared<Texture>(resolved_path);
    this->intensity = intensity;
    this->scene_center = scene_center;
    this->scene_radius = scene_radius;
}

void SkyLight::commit() {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

std::tuple<glm::vec3, Ray, float> SkyLight::sample_Li(const SurfaceInteraction& hit, const glm::vec2& sample) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

float SkyLight::pdf_Li(const SurfaceInteraction& light, const Ray& ray) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

glm::vec3 SkyLight::Le(const Ray& ray) const {
    assert(tex && distribution);
    return tex->env(ray.dir) * intensity;
}

std::tuple<glm::vec3, Ray, glm::vec3, float, float> SkyLight::sample_Le(const glm::vec2& sample_pos, const glm::vec2& sample_dir) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

std::tuple<float, float> SkyLight::pdf_Le(const SurfaceInteraction& light, const glm::vec3& dir) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

glm::vec3 SkyLight::power() const {
    assert(tex && distribution);
    return glm::vec3(PI * scene_radius * scene_radius * intensity * distribution->unit_integral());
}

inline std::string fix_data_path(std::string path) {
    // remove GI_DATA_DIR from path to avoid absolute paths
    if (path.find(GI_DATA_DIR) != std::string::npos)
        path = path.substr(path.find(GI_DATA_DIR) + std::strlen(GI_DATA_DIR) + 1);
    return path;
}

json11::Json SkyLight::to_json() const {
    return json11::Json::object{
        { "envmap", fix_data_path(tex->path().string()) },
        { "intensity", intensity },
        { "scene_center", json11::Json::array{scene_center.x, scene_center.y, scene_center.z} },
        { "scene_radius", scene_radius },
    };
}

void SkyLight::from_json(const json11::Json& cfg) {
    if (cfg.is_object()) {
        json_set_float(cfg, "intensity", intensity);
        json_set_vec3(cfg, "scene_center", scene_center);
        json_set_float(cfg, "scene_radius", scene_radius);
        if (cfg["envmap"].is_string())
            load(cfg["envmap"].string_value(), scene_center, scene_radius, intensity);
        else
            tex = std::make_shared<Texture>(glm::vec3(1));
        commit();
    }
}
