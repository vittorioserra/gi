#include "framebuffer.h"
#include "texture.h"
#include "timer.h"
#include "color.h"
#include <iostream>
#include <atomic>
#include <cmath>
#include <omp.h>

// mitchell filter
inline float mitchell(float x, float B = 0.5, float C = 0.25) {
    x = std::abs(2 * x);
    if (x > 1)
        return ((-B - 6*C) * x*x*x + (6*B + 30*C) * x*x +
                (-12*B - 48*C) * x + (8*B + 24*C)) * (1.f/6.f);
    else
        return ((12 - 9*B - 6*C) * x*x*x +
                (-18 + 12*B + 6*C) * x*x +
                (6 - 2*B)) * (1.f/6.f);
}

// lanczos filter
inline float sinc(float x) {
    x = std::abs(x);
    if (x < 1e-5) return 1.f;
    return std::sin(M_PI * x) / (M_PI * x);
}
inline float windowed_sinc(float x, float radius, float tau = 3) {
    x = std::abs(x);
    if (x > radius) return 0.f;
    return sinc(x) * sinc(x / tau);
}

// gaussian filter
inline float gaussSigma(float d, float sigma = 3.f) {
    return (d * d) / (2 * sigma * sigma);
}
inline float gaussSigma(const glm::vec2& d, float sigma = 3.f) {
    return dot(d, d) / (2 * sigma * sigma);
}

// bilateral gaussian filter
inline float bilateral(const glm::vec2& d_pixel, const glm::vec3& d_color) {
    const float dist = gaussSigma(d_pixel);
    const float range = gaussSigma(luma(d_color));
    return exp(-dist - range);
}

inline glm::vec3 finite_fix(const glm::vec3& v) {
    return glm::vec3(std::isfinite(v.x) ? v.x : 0.f, std::isfinite(v.y) ? v.y : 0.f, std::isfinite(v.z) ? v.z : 0.f);
}

// -----------------------------------------------------------------
// Framebuffer

Framebuffer::Framebuffer(size_t w, size_t h, size_t sppx) 
    : w(w), h(h), sppx(sppx), color(w, h), num_samples(w, h), even(w, h), fbo(w, h) {
    clear();
#ifdef WITH_OIDN
    device = oidn::newDevice();
    device.commit();
#endif
}

void Framebuffer::clear() {
    color = glm::vec3(0);
    num_samples = 0;
    even = glm::vec3(0);
    fbo = glm::vec3(0);
}

void Framebuffer::resize(size_t w, size_t h, size_t sppx) {
    this->w = w;
    this->h = h;
    this->sppx = sppx;
    color.resize(w, h);
    fbo.resize(w, h);
    num_samples.resize(w, h);
    even.resize(w, h);
    clear();
}

void Framebuffer::add_sample(size_t x, size_t y, const glm::vec3& irradiance) {
    assert(x < w); assert(y < h);
    STAT("fbo add sample");
    // add sample
    num_samples(x, y)++;
    const glm::vec3 fix = glm::clamp(finite_fix(irradiance), 0.f, 100.f);
    const glm::vec3 add = HDR ? fix : glm::clamp(EXPOSURE * hableTonemap(fix), 0.f, 1.f);
    color(x, y) = glm::mix(color(x, y), add, 1.f / num_samples(x, y));
    if (num_samples(x, y) % 2 == 0)
        even(x, y) = glm::mix(even(x, y), add, 1.f / (num_samples(x, y) / 2));
    // push update
    if (PREVIEW_CONV)
        fbo(x, y) = heatmap(luma(glm::abs(color(x, y) - even(x, y))) / fmaxf(FLT_EPSILON, luma(color(x, y))));
    else
        fbo(x, y) = HDR ? EXPOSURE * hableTonemap(color(x, y)) : color(x, y);
}

void Framebuffer::show_convergence() {
    PREVIEW_CONV = true;
#if defined(__unix__)
    #pragma omp parallel for
#endif
    for (size_t y = 0; y < h; ++y)
        for (size_t x = 0; x < w; ++x)
            fbo(x, y) = heatmap(luma(glm::abs(color(x, y) - even(x, y))) / fmaxf(FLT_EPSILON, luma(color(x, y))));
}

void Framebuffer::show_num_samples() {
    size_t n_min = UINT_MAX, n_max = 0, n_sum = 0;
#if defined(__unix__)
    #pragma omp parallel for reduction(max : n_max) reduction(min : n_min) reduction(+ : n_sum)
#endif
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            n_max = glm::max(n_max, num_samples(x, y));
            n_min = glm::min(n_min, num_samples(x, y));
            n_sum += num_samples(x, y);
        }
    }
#if defined(__unix__)
    #pragma omp parallel for
#endif
    for (size_t y = 0; y < h; ++y)
        for (size_t x = 0; x < w; ++x)
            fbo(x, y) = heatmap(num_samples(x, y) / float(n_max));
    printf("(sppx: %lu, min: %lu, max: %lu, avg: %lu)\n", sppx, n_min, n_max, size_t(n_sum / float(w*h)));
}

void Framebuffer::tonemap() {
    PREVIEW_CONV = false;
    PREVIEW_EXPOSURE = 1.f;
#if defined(__unix__)
    #pragma omp parallel for
#endif
    for (size_t y = 0; y < h; ++y)
        for (size_t x = 0; x < w; ++x)
            fbo(x, y) = HDR ? EXPOSURE * hableTonemap(color(x, y)) : color(x, y);
}

#ifdef WITH_OIDN
void Framebuffer::denoise() {
	std::cout << "denoising..." << std::endl;
    auto input = fbo;
    oidn::FilterRef filter = device.newFilter("RT"); // generic ray tracing filter
    filter.setImage("color", &input[0], oidn::Format::Float3, w, h);
    filter.setImage("output", &fbo[0], oidn::Format::Float3, w, h);
    filter.set("hdr", false);
    filter.commit();
    filter.execute();
    const char* errorMessage;
    if (device.getError(errorMessage) != oidn::Error::None)
        std::cout << "Error: " << errorMessage << std::endl;
}
#endif

float Framebuffer::geo_mean_luma() const {
    float log_accum = 0;
#if defined(__unix__)
    #pragma omp parallel for reduction(+ : log_accum)
#endif
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            const float f = fmaxf(0.f, logf(fmaxf(1e-4, color(x, y).y)));
            if (std::isfinite(f)) log_accum += f;
        }
    }
    return expf(log_accum / float(w * h));
}

void Framebuffer::save(const std::filesystem::path& path) const {
    if (path.extension() == ".png")
        Texture::save_png(path, w, h, fbo.data());
    else if (path.extension() == ".jpg" || path.extension() == ".jpeg")
        Texture::save_jpg(path, w, h, fbo.data());
    else {
        std::cerr << "Warning: Framebuffer::save(): unsupported file extension, falling back to PNG." << std::endl;
        std::filesystem::path p = path;
        Texture::save_png(p.replace_extension(".png"), w, h, fbo.data());
    }
}

json11::Json Framebuffer::to_json() const {
    return json11::Json::object {
        { "res_w", int(w) },
        { "res_h", int(h) },
        { "sppx", int(sppx) },
        { "hdr", HDR },
        { "exposure", EXPOSURE }
    };
}

void Framebuffer::from_json(const json11::Json& cfg) {
    if (cfg.is_object()) {
        json_set_size(cfg, "res_w", w);
        json_set_size(cfg, "res_h", h);
        json_set_size(cfg, "sppx", sppx);
        json_set_bool(cfg, "hdr", HDR);
        json_set_float(cfg, "exposure", EXPOSURE);
        // apply changes
        resize(w, h, sppx);
    }
}
