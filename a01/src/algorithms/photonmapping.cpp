#include "driver/context.h"
#include "gi/algorithm.h"
#include "gi/bdpt.h"
#include "photon_map.h"

using namespace glm;

// TODO: clean defines

// -------------------------------------
// Photon mapping helper functions

// shade point on camera path (direct illum)
vec3 direct_illum(Context& context, const SurfaceInteraction& hit, const vec3& w_o) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// get radiance estimate at given point from given photon map
vec3 radiance_estimate(Context& context, const SurfaceInteraction& hit, const vec3& w_o, const PhotonMap& photon_map, size_t n_photons) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// perform final gathering to properly capture smooth indirect illum
vec3 final_gather(Context& context, const SurfaceInteraction& hit, const vec3& w_o, const PhotonMap& photon_map) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

// -------------------------------------
// Photon mapping algorithm

struct PhotonMapping : public Algorithm {
    inline static const std::string name = "PhotonMapping";

    // Photon mapping parameters: trade quality for performance here
    const uint32_t NUM_PHOTON_PATHS = 1 << 18;
    const bool DIRECT_VISUALIZATION = false;

    // called once before each(!) rendering
    void init(Context& context) {
        throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
    }

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
    }

    // data
    PhotonMap photon_map;
};

static AlgorithmRegistrar<PhotonMapping> registrar;
