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

        StratifiedSampler2D lens_sample = StratifiedSampler2D();

        lens_sample.init(samples);


        std::vector<Ray> aa_ray(samples);

        vector<glm::vec2> jitter(samples);
        vector<glm::vec2> lens_dof(samples);


        for(int i = 0; i < samples; i++){

            jitter[i] = halton_samp.next();
            lens_dof[i] = lens_sample.next();

            aa_ray[i] = cam.view_ray(x, y, w, h, jitter[i], lens_dof[i]);

        }
        vec3 L(0);


        StratifiedSampler2D samp_source_s = StratifiedSampler2D();
        StratifiedSampler2D samp_area_s = StratifiedSampler2D();

        samp_source_s.init(samples);
        samp_area_s.init(samples);

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

                const auto [light_ptr, ignore_me] = scene.sample_light_source(samp_source_s.next()[0]);

                /*

                if(!light_ptr){

                    std::cout<< "NULL light pointer" << std::endl;
                    L = hit.albedo();//Li;

                }else{

                auto [Li, shadow_ray, ignore_me2] = light_ptr->sample_Li(hit, samp_area_s.next());

                bool occluded = scene.occluded(shadow_ray);

                if(!occluded){

                    L = Li;


                }



                L = glm::vec3(0.0, 0.0, 0.0);

                }
                */

                auto [Li, shadow_ray, ignore_me2] = light_ptr->sample_Li(hit, samp_area_s.next());

                bool occluded = scene.occluded(shadow_ray);

                if(!occluded){


                    //glm::vec3 omega_i = -glm::normalize(light_ptr->P - hit.P);
                    glm::vec3 n = hit.N;

                    float cos_term = glm::dot(shadow_ray.dir, n);


                    if(cos_term < 0.0){

                        cos_term = 0.0;

                    }

                    //std::cout<< " cos term : " << cos_term << std::endl;

                    //L = cos_term * Li * hit.albedo();

                    L = cos_term * Li * hit.albedo();


                }else{

                    L = glm::vec3(0.0,0.0, 0.0);

                }





                //L = hit.albedo();
                }
        } else // ray esacped the scene
                    L = scene.Le(aa_ray[i]);
                // add result to framebuffer

        }

                fbo.add_sample(x, y, L);

        /*

        // setup a view ray
        Ray ray = cam.view_ray(x, y, w, h);
        // intersect main ray with scene
        const SurfaceInteraction hit = scene.intersect(ray);
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
                L = hit.albedo();
                }
        } else // ray esacped the scene
                    L = scene.Le(ray);
                // add result to framebuffer
                fbo.add_sample(x, y, L);*/
            }
        };

static AlgorithmRegistrar<SimpleRenderer> registrar;
