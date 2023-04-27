#include "bdpt.h"
#include "driver/context.h"
#include "light.h"
#include "material.h"
#include "mesh.h"
#include "color.h"
#include <mutex>

void trace_cam_path(const Context& context, uint32_t x, uint32_t y, std::vector<PathVertex>& cam_path,
        RandomWalkCam& walk, const bool specular_path_tracing) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

void trace_light_path(const Context& context, std::vector<PathVertex>& light_path, RandomWalkLight& walk) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

glm::vec3 connect_and_shade(const Context& context, const std::vector<PathVertex>& cam_path, const std::vector<PathVertex>& light_path) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}

void trace_photons(const Context& context, int N, std::vector<PathVertex>& photons, bool scale_photon_power) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
}
