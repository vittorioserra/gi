#include "driver/context.h"
#include "gi/algorithm.h"
#include "gi/surface.h"
#include "gi/scene.h"
#include "gi/random.h"
#include "gi/light.h"
#include "gi/ray.h"

using namespace std;
using namespace glm;

struct BRDFImportance : public Algorithm {
    inline static const std::string name = "BRDFImportance";

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
    }
};

static AlgorithmRegistrar<BRDFImportance> registrar;
