#include "camera.h"
#include "sampling.h"
#include "timer.h"
#include <iostream>

// -----------------------------------------------------
// Implementation

void Camera::commit() {
    dir = glm::normalize(dir);
    up = glm::normalize(up);
    const glm::vec3 r = glm::normalize(glm::cross(dir, up + glm::vec3(0.0001f, 0.f, 0.f)));
    up = glm::normalize(glm::cross(r, dir));
    eye_to_world = glm::mat3(r, up, -dir);
}

Ray Camera::view_ray(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const glm::vec2& pixel_sample, const glm::vec2& lens_sample) const {
    assert(pixel_sample.x >= 0 && pixel_sample.x < 1); assert(pixel_sample.y >= 0 && pixel_sample.y < 1);
    assert(lens_sample.x >= 0 && lens_sample.x < 1); assert(lens_sample.y >= 0 && lens_sample.y < 1);
    STAT("setup view ray");
    // generate perspective or environment ray
    Ray view_ray = perspective ?
        perspective_view_ray(x, y, w, h, pixel_sample) :
        environment_view_ray(x, y, w, h, pixel_sample);
    // add DOF?
    if (lens_radius > 0 && lens_sample != glm::vec2(.5))
        apply_DOF(view_ray, lens_sample);
    return view_ray;
}

Ray Camera::perspective_view_ray(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const glm::vec2& pixel_sample) const {
    // TODO ASSIGNMENT1
    // jitter the ray throughout the pixel (x, y) using pixel_sample

    glm::vec2 pixel_jitter = pixel_sample;

    //std::cout << pixel_sample[0] << " , " << pixel_sample[1] << std::endl;

    const glm::vec2 pixel = glm::vec2(x, y) +  pixel_jitter;
    const glm::vec2 ndch = (pixel - glm::vec2(w * .5f, h * .5f)) / glm::vec2(h);
    const float z = -.5f / tanf(.5f * M_PI * fov / 180.f);
    return Ray(pos, eye_to_world * glm::normalize(glm::vec3(ndch.x, ndch.y, z)));
}

Ray Camera::environment_view_ray(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const glm::vec2& pixel_sample) const {
    const float theta = M_PI * (y + pixel_sample.y) / float(h);
    const float phi = 2 * M_PI * (x + pixel_sample.x) / float(w);
    return Ray(pos, glm::vec3(sinf(theta) * cosf(phi), -cosf(theta), sinf(theta) * sinf(phi)));
}

void Camera::apply_DOF(Ray& ray, const glm::vec2& lens_sample) const {
    // shift ray origin on (thin) lens
    const glm::vec3 view_dir = this->dir;
    const auto [tangent, bitangent] = build_tangent_frame(view_dir);
    const glm::vec2 p_on_lens = uniform_sample_disk(lens_sample); // mapping from unit quad to unit circle
    // TODO ASSIGNMENT1
    // add DOF to the given view ray (in-place)
    // thus, jitter the ray origin on the (circular) thin lens and update the ray's direction to the focal point
    // hint: use the coordinate system given via the tangent and bitangent to jitter the ray's origin
    // hint: the lens size is given in this->lens_radius and the focal distance in this->focal_depth

    glm::vec3 jitter_dir_u = glm::normalize(tangent)  * (p_on_lens[0])* this->lens_radius;
    glm::vec3 jitter_dir_v = glm::normalize(bitangent)  * (p_on_lens[1])* this->lens_radius;

    glm::vec3 general_jitter = jitter_dir_u + jitter_dir_v;

    /*float ft_x = this->focal_depth / view_dir.z;
    float ft_y = this->focal_depth / view_dir.z;
    float ft_z = this->focal_depth / view_dir.z;*/

    glm::vec3 focal_point = ray.org + glm::normalize(ray.dir)*focal_depth/glm::dot(glm::normalize(ray.dir), glm::normalize(view_dir)); //view_dir

    glm::vec3 old_org = ray.org;

    ray.org = ray.org + general_jitter;
    ray.dir = glm::normalize(focal_point - ray.org);

}

json11::Json Camera::to_json() const {
    return json11::Json::object {
        { "pos", json11::Json::array{ pos.x, pos.y, pos.z } },
            { "dir", json11::Json::array{ dir.x, dir.y, dir.z } },
            { "up", json11::Json::array{ up.x, up.y, up.z } },
            { "fov", fov },
            { "lens_radius", lens_radius },
            { "focal_depth", focal_depth }
    };
}

void Camera::from_json(const json11::Json& cfg) {
    if (cfg.is_object()) {
        json_set_vec3(cfg, "pos", pos);
        json_set_vec3(cfg, "dir", dir);
        json_set_vec3(cfg, "up", up);
        json_set_float(cfg, "fov", fov);
        json_set_float(cfg, "lens_radius", lens_radius);
        json_set_float(cfg, "focal_depth", focal_depth);
        commit();
    }
}
