#pragma once
#include "light.h"
#include "material.h"
#include "mesh.h"
#include "ray.h"
#include "sampling.h"
#include <cmath>
#include <tuple>

/**
 * @brief The SurfaceInteraction class provides an abstraction layer over surface interactions and materials.
 */
class SurfaceInteraction {
public:
    /**
     * @brief Default construct as invalid surface interaction with optional sky light contribution
     */
    SurfaceInteraction(const SkyLight* sky = 0);

    /**
     * @brief Construct as ray surface interaction and perform hit point interpolation
     *
     * @param ray Ray that hit something
     * @param mesh Pointer to mesh which the ray hit
     */
    SurfaceInteraction(const Ray& ray, const Mesh* mesh);

    /**
     * @brief Construct as mesh sample, e.g. when sampling a mesh light source
     *
     * @param sample Random sample in [0,1)
     * @param primID ID of the primitive to be sampled
     * @param mesh Pointer to the mesh to be sampled
     */
    SurfaceInteraction(const glm::vec2& sample, uint32_t primID, const Mesh* mesh);

    /**
     * @brief Construct as abstract surface without mesh or material, e.g. a camera sample for BDPT
     *
     * @param pos Surface position
     * @param norm Surface normal
     */
    SurfaceInteraction(const glm::vec3& pos, const glm::vec3& norm);

    // data
    const bool valid;    ///< Use this to check for a valid surface interaction
    glm::vec3 P;         ///< World space position
    glm::vec3 Ng;        ///< World space geometry normal
    glm::vec3 N;         ///< World space shading normal (including normalmapping)
    glm::vec2 TC;        ///< Texture coordinates, or glm::vec2(0) if none available
    float area;          ///< Hit primitive surface area
    const Mesh* mesh;    ///< Mesh pointer, may be 0 for abstract surfaces
    const Material* mat; ///< Material pointer, may be 0 for abstract surfaces
    const Light* light;  ///< Light source pointer, set if a light source was hit (type is: valid ? AreaLight : SkyLight)

    /**
     * @brief Evaluate the BRDF of this surface interaction
     *
     * @param w_o World space out direction
     * @param w_i World space in direction
     *
     * @return BRDF value
     */
    glm::vec3 brdf(const glm::vec3& w_o, const glm::vec3& w_i) const;

    /**
     * @brief Sample the BRDF of this surface interaction
     *
     * @param w_o World space out direction
     * @param w_i Ref to glm::vec3, where the sampled world space in direction will be written to
     * @param sample Random sample used for sampling the BRDF
     * @param pdf Ref to float, where the pdf value of sampled direction will be written to
     *
     * @return BRDF value of sampled direction
     */
    std::tuple<glm::vec3, glm::vec3, float> sample(const glm::vec3& w_o, const glm::vec2& sample) const;

    /**
     * @brief Evaluate the pdf of this surface interaction
     *
     * @param w_o World space out direction
     * @param w_i World space in direction
     *
     * @return pdf value
     */
    float pdf(const glm::vec3& w_o, const glm::vec3& w_i) const;

    /**
     * @brief Spawn ray from this surface interaction, with offset along normal to avoid self intersections
     *
     * @param dir Outgoing ray direction
     * @param len Outgoing ray length
     *
     * @return Ray in direction dir with offset
     */
    inline Ray spawn_ray(const glm::vec3& dir, float len = FLT_MAX) const { return Ray(P, dir, len); }

    /**
     * @brief Fetch surface color (albedo)
     *
     * @return albedo, from either static color or texture map
     */
    inline glm::vec3 albedo() const
    {
        assert(mat);
        return mat->albedo(TC);
    }

    /**
     * @brief Fetch surface roughness
     *
     * @return roughness, from either static value or texture
     */
    inline float roughness() const
    {
        assert(mat);
        return mat->roughness(TC);
    }

    /**
     * @brief Query if surface is a light source
     *
     * @return true, if light source, false otherwise
     */
    inline bool is_light() const { return light != 0; }

    /**
     * @brief Query for emitted light
     *
     * @return emitted light, either from static color or texture
     */
    inline glm::vec3 Le() const
    {
        assert(mat);
        return mat->emissive(TC);
    }

    /**
     * @brief Query if surface is of given type
     *
     * @param type BRDFType to check surface against
     *
     * @return true if surface is of given type, false otherwise
     */
    inline bool is_type(const BRDFType& type) const
    {
        assert(mat);
        return mat->brdf->is_type(type);
    }

    /**
     * @brief Conversion from world to tangent space
     *
     * @param world_dir World space direction
     *
     * @return Tangent space direction
     */
    glm::vec3 to_tangent(const glm::vec3& world_dir) const
    {
        assert(valid);
        return world_to_tangent(N, world_dir);
    }

    /**
     * @brief Conversion from tangent to world space direction
     *
     * @param tangent_dir Tangent space direction
     *
     * @return World space direction
     */
    glm::vec3 to_world(const glm::vec3& tangent_dir) const
    {
        assert(valid);
        return align(N, tangent_dir);
    }
};
