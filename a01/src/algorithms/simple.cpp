#include "driver/context.h"
#include "gi/algorithm.h"
#include "gi/surface.h"
#include "gi/scene.h"
#include "gi/random.h"
#include "gi/light.h"
#include "gi/ray.h"


using namespace std;
using namespace glm;

struct SimpleRenderer : public Algorithm {
    inline static const std::string name = "SimpleRenderer";

    void sample_pixel(Context& context, uint32_t x, uint32_t y, uint32_t samples) {
        // some shortcuts
        const Camera& cam = context.cam;
        const Scene& scene = context.scene;
        Framebuffer& fbo = context.fbo;
        const size_t w = fbo.width(), h = fbo.height();

        // TODO ASSIGNMENT1
        // - add and initialize random samplers
        // - apply supersampling over #samples and DOF using your samplers

        uint32_t img_size = w * h;

        UniformSampler2D uniform_samp = UniformSampler2D();
        StratifiedSampler2D strat_samp = StratifiedSampler2D();
        HaltonSampler2D halton_samp = HaltonSampler2D();
        HammersleySampler2D hammer_samp = HammersleySampler2D();
        LDSampler2D ld_samp = LDSampler2D();

        uniform_samp.init(samples);
        strat_samp.init(samples);
        halton_samp.init(samples);
        hammer_samp.init(samples);
        ld_samp.init(samples);


        HaltonSampler2D lens_sample = HaltonSampler2D();

        lens_sample.init(samples);


        std::vector<Ray> aa_ray(samples);

        std::vector<glm::vec2> jitter(samples);
        std::vector<glm::vec2> lens_dof(samples);


        for(int i = 0; i < samples; i++){

            jitter[i] = halton_samp.next();
            lens_dof[i] = lens_sample.next();

            aa_ray[i] = cam.view_ray(x, y, w, h, jitter[i], lens_dof[i]);

        }
        vec3 L(0);


        HaltonSampler2D samp_source_s = HaltonSampler2D();
        HaltonSampler2D samp_area_s = HaltonSampler2D();

        StratifiedSampler1D samp_light_source = StratifiedSampler1D();

        samp_source_s.init(samples);
        samp_area_s.init(samples);

        samp_light_source.init(samples);

        for(int i = 0; i < samples; i ++){

            const SurfaceInteraction hit = scene.intersect(aa_ray[i]);
        // check if a hit was found
        if (hit.valid) {
            if (hit.is_light()) // direct light source hit
                L = hit.Le();
            else { // surface hit -> shading

                // TODO ASSIGNMENT1
                // add area light shading via the rendering equation from the assignment sheet
                // hint: use the following c++17 syntax to capture multiple return values:
                // const auto [light_ptr, ignore_me] = scene.sample_light_source(...);
                // auto [Li, shadow_ray, ignore_me2] = light_ptr->sample_Li(...);

                const auto [light_ptr, ignore_me] = scene.sample_light_source(samp_light_source.next());


                auto [Li, shadow_ray, ignore_me2] = light_ptr->sample_Li(hit, samp_area_s.next());

                bool occluded = scene.occluded(shadow_ray);

                if(!occluded){


                    glm::vec3 n = hit.N;

                    float cos_term = glm::dot(glm::normalize(shadow_ray.dir), glm::normalize(n));


                    if(cos_term < 0.0f){

                        cos_term = 0.0f;

                    }


                    L = cos_term* Li * hit.albedo();


                }else{

                    L = glm::vec3(0.0f,0.0f,0.0f);

                }

            }



        }
        else{ // ray esacped the scene
                L = scene.Le(aa_ray[i]);}
                // add result to framebuffer
                fbo.add_sample(x, y, L);

        }

            }
        };

static AlgorithmRegistrar<SimpleRenderer> registrar;
