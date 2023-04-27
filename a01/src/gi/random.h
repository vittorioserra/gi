#pragma once

#include "rng.h"
#include "timer.h"
#include "buffer.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <glm/glm.hpp>

// --------------------------------------------------------------------------------
// Random sampler interface

/**
 * @brief Random sampler interface with generic type
 * @note Returned samples must always be in the [0, 1) range.
 *
 * @tparam T Type of the returned samples, e.g. float, glm::vec2, etc.
 */
template <typename T> class Sampler {
public:
    typedef T return_t;

    /**
     * @brief Initialize this sampler to N samples
     *
     * @param N Number of samples that will be drawn from this sampler, via N calls to next()
     */
    virtual void init(uint32_t N) = 0;

    /**
     * @brief Draw the next sample
     *
     * @return The next sample of type T from this distribution 
     */
    virtual T next() = 0;
};

// --------------------------------------------------------------------------------
// Collection of pseudorandom sampling routines

/**
 * @brief Halton low discrepancy sequence (radical inverse)
 *
 * @param i i-th number to draw from this sequence
 * @param base Base for radical inverse computation
 *
 * @return i-th sample of this sequence
 */
inline float halton(int i, uint32_t base) {
    float result = 0;
    float f = 1.f / ((float)base);
    while (i > 0) {
        result = result + (f * (i % base));
        i = i / base;
        f = f / ((float)base);
    }
    return result;
}

/**
 * @brief Van der Corput low discrepancy sequence
 *
 * @param i i-th number to draw from this sequence
 * @param scramble Seed to scramble the distribution
 *
 * @return i-th sample of this sequence
 */
inline float vandercorput(uint32_t i, uint32_t scramble) {
    i = (i << 16) | (i >> 16);
    i = ((i & 0x00ff00ff) << 8) | ((i & 0xff00ff00) >> 8);
    i = ((i & 0x0f0f0f0f) << 4) | ((i & 0xf0f0f0f0) >> 4);
    i = ((i & 0x33333333) << 2) | ((i & 0xcccccccc) >> 2);
    i = ((i & 0x55555555) << 1) | ((i & 0xaaaaaaaa) >> 1);
    i ^= scramble;
    return ((i >> 8) & 0xffffff) / float(1 << 24);
}

/**
 * @brief Hammersley set low discrepancy sequence
 *
 * @param i i-th number to draw from this sequence
 * @param n Total number of samples to draw from this sequence
 * @param scramble Seed to scramble the distribution
 *
 * @return i-th sample of this sequence
 */
inline glm::vec2 hammersley(uint32_t i, uint32_t n, uint32_t scramble) {
    return glm::vec2(float(i) / float(n), vandercorput(i, scramble));
}

/**
 * @brief Sobol low discrepancy sequence
 *
 * @param i i-th number to draw from this sequence
 * @param scramble Seed to scramble the distribution
 *
 * @return i-th sample of this sequence
 */
inline float sobol2(uint32_t i, uint32_t scramble) {
    for (uint32_t v = 1 << 31; i != 0; i >>= 1, v ^= v >> 1)
        if (i & 0x1)
            scramble ^= v;
    return ((scramble >> 8) & 0xffffff) / float(1 << 24);
}

/**
 * @brief 0-2 low discrepancy sequence
 *
 * @param i i-th number to draw from this sequence
 * @param scramble[2] Seeds to scramble the distributions
 *
 * @return i-th sample of this sequence
 */
inline glm::vec2 sample02(uint32_t i, const uint32_t scramble[2]) {
    return glm::vec2(vandercorput(i, scramble[0]), sobol2(i, scramble[1]));
}

// --------------------------------------------------------------------------------
// 1D sampler implementations

class UniformSampler1D : public Sampler<float> {
public:
    inline void init(uint32_t N) {}

    inline float next() {
        STAT("random sampling");
        return RNG::uniform<float>();
    }
};

class StratifiedSampler1D : public Sampler<float> {
public:

    float dist;
    float pos;

    inline void init(uint32_t N) {
        // TODO
        dist = 1.0/(float)N;
        pos = 0.5;
    }

    inline float next() {
        // TODO ASSIGNMENT1
        // return the next stratified sample
        STAT("stratified sampling");

        float random_uniform_val = RNG::uniform<float>();

        //std::cout << dist << " N value"<< std::endl;

        float sample_pos = dist * random_uniform_val + dist*pos;
        pos = pos + 1.0;

        return sample_pos;

        //std::cout << random_uniform_val << std::endl;
    }
};

// --------------------------------------------------------------------------------
// 2D sampler implementations

class UniformSampler2D : public Sampler<glm::vec2> {
public:
    inline void init(uint32_t N) {}

    inline glm::vec2 next() {
        STAT("random sampling");
        return RNG::uniform<glm::vec2>();
    }
};

class StratifiedSampler2D : public Sampler<glm::vec2> {
public:

    float dist;
    int pos_linear;
    int n_pixel_line;

    inline void init(uint32_t N) {
        // TODO ASSIGNMENT1
        // note: you may assume N to be quadratic
        dist = (float) N; //std::sqrt((float)N);
        dist = std::sqrt(dist);
        n_pixel_line = (int)dist;

        dist = 1.0/dist;
        pos_linear = 0;
    }
    inline glm::vec2 next() {
        // TODO ASSIGNMENT1
        // return the next stratified sample

        int pos_x = pos_linear % n_pixel_line;
        int pos_y = pos_linear / n_pixel_line;

        //std::cout<< n_pixel_line <<std::endl;

        //std::cout<< pos_x << " " << pos_y << std::endl;

        float value_1 = dist * RNG::uniform<float>()  + pos_x/(float)n_pixel_line ;//+ 0.5*dist;
        float value_2 = dist * RNG::uniform<float>()  + pos_y/(float)n_pixel_line ;//+ 0.5*dist;

        pos_linear ++;

        //std::cout << value_1 << " " << value_2 <<std::endl;

        //std::cout<< dist<<std::endl;

        return glm::vec2(value_1, value_2);
    }
};

class HaltonSampler2D : public Sampler<glm::vec2> {
public:

    float x_rand_halton;
    float y_rand_halton;

    float dist;
    int pos_linear;
    int n_pixel_line;

    inline void init(uint32_t N) {
        // TODO ASSIGNMENT1
        // note: bases 2 and 3 are commonly used
        dist = (float) N; //std::sqrt((float)N);
        dist = std::sqrt(dist);
        n_pixel_line = (int)dist;

        dist = 1.0/dist;
        pos_linear = 0;
    }

    inline glm::vec2 next() {
        // TODO ASSIGNMENT1
        // note: see helper function halton() above

        int pos_x = pos_linear % n_pixel_line;
        int pos_y = pos_linear / n_pixel_line;

        //std::cout<< n_pixel_line <<std::endl;

        //std::cout<< pos_x << " " << pos_y << std::endl;

        float value_1 = halton(pos_linear, 2);//+ pos_x/(float)n_pixel_line + 0.5*dist;
        float value_2 = halton(pos_linear, 3);//  + pos_y/(float)n_pixel_line + 0.5*dist;

        //std::cout<<halton(pos_linear, 2) << " " << halton(pos_linear, 3) << std::endl;

        pos_linear ++;

        return glm::vec2(value_1, value_2);
    }
};

class HammersleySampler2D : public Sampler<glm::vec2> {
public:

    int seed;

    float dist;
    int pos_linear;
    int n_pixel_line;
    int n_pixel_tot;

    inline void init(uint32_t N) {
        // TODO ASSIGNMENT1
        // note: use a random seed
        dist = (float) N; //std::sqrt((float)N);
        dist = std::sqrt(dist);
        n_pixel_line = (int)dist;

        n_pixel_tot = N;

        dist = 1.0/dist;
        pos_linear = 0;

        seed = 42;
    }

    inline glm::vec2 next() {
        // TODO ASSIGNMENT1
        // note: see helper function hammersley() above
        glm::vec2 sample = hammersley(pos_linear, n_pixel_tot,seed);//+ pos_x/(float)n_pixel_line + 0.5*dist;

        //std::cout<<sample[0] << " " <<sample[1]<<std::endl;

        pos_linear = pos_linear + 1;

        return sample;
    }
};

class LDSampler2D : public Sampler<glm::vec2> {
public:

    uint32_t seeds[2];

    float dist;
    int pos_linear;
    int n_pixel_line;
    int n_pixel_tot;

    inline void init(uint32_t N) {
        // TODO ASSIGNMENT1
        // note: use two random seeds
        dist = (float) N; //std::sqrt((float)N);
        dist = std::sqrt(dist);
        n_pixel_line = (int)dist;

        n_pixel_tot = N;

        dist = 1.0/dist;
        pos_linear = 0;

        seeds[0] = 41;
        seeds[1] = 42;
    }

    inline glm::vec2 next() {
        // TODO ASSIGNMENT1
        // note: see helper function sample02() above
        glm::vec2 sample = sample02(pos_linear, seeds);//+ pos_x/(float)n_pixel_line + 0.5*dist;

        std::cout<<sample[0] << " " <<sample[1]<<std::endl;


        pos_linear = pos_linear + 1;
        return sample;
    }
};

// --------------------------------------------------------------------------------
// Shuffe sampler, precompute and shuffle samples

template <typename Sampler>
class ShuffleSampler {
public:
    ShuffleSampler(uint32_t N) : samples(N) {
        Sampler s;
        s.init(N);
        for (uint32_t i = 0; i < N; ++i) samples[i] = s.next();
        RNG::shuffle(samples);
    }

    typename Sampler::return_t operator[](uint32_t i) { return samples[i]; }

    std::vector<typename Sampler::return_t> samples;
};

// --------------------------------------------------------------------------------
// Debugging utilities

inline void plot_samples(Sampler<glm::vec2>& sampler, const std::string& filename, uint32_t N = 1024) {
    // compute and plot samples
    const uint32_t w = 512, h = 512;
    Buffer<glm::vec3> buffer(w, h);
    buffer = glm::vec3(0);
    sampler.init(N);
    for (uint32_t i = 0; i < N; ++i) {
        const glm::vec2 sample = sampler.next();
        assert(sample.x >= 0 && sample.x < 1); assert(sample.y >= 0 && sample.y < 1);
        const int d = 2; // pixel width
        for (int dx = -d; dx <= d; ++dx) {
            for (int dy = -d; dy <= d; ++dy) {
                uint32_t x = sample.x * w + dx;
                uint32_t y = sample.y * h + dy;
                if (x >= 0 && y >= 0 && x < w && y < h)
                    buffer(x, y) = glm::vec3(i / float(N), 1 - i / float(N), 1 - i / float(N));
            }
        }
    }
    // output
    Texture::save_png(filename, w, h, buffer.data());
}

inline void plot_all_samplers2D() {
    UniformSampler2D uni;
    StratifiedSampler2D strat;
    HaltonSampler2D halt;
    HammersleySampler2D hamm;
    LDSampler2D ld;
    plot_samples(uni, "uniform1.png");
    plot_samples(uni, "uniform2.png");
    plot_samples(uni, "uniform3.png");
    plot_samples(strat, "stratified1.png");
    plot_samples(strat, "stratified2.png");
    plot_samples(strat, "stratified3.png");
    plot_samples(halt, "halton1.png");
    plot_samples(halt, "halton2.png");
    plot_samples(halt, "halton3.png");
    plot_samples(hamm, "hammersley1.png");
    plot_samples(hamm, "hammersley2.png");
    plot_samples(hamm, "hammersley3.png");
    plot_samples(ld, "low-discrepancy1.png");
    plot_samples(ld, "low-discrepancy2.png");
    plot_samples(ld, "low-discrepancy3.png");
}

template <typename T> void sampler_benchmark(uint32_t N) {
    // make gcc not optimize away the call
    static glm::vec2 result;
    // init
    T sampler;
    sampler.init(N);
    // benchmark
    const auto start = std::chrono::system_clock::now();
    for (uint32_t i = 0; i < N; ++i)
        result += sampler.next();
    const double dur = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - start).count();
    std::cout << "avg ns: " << dur / N << std::endl;
}

inline void perform_sampler_benchmarks(uint32_t num_samples = 1000000) {
    std::cout << "Sampler benchmarks using " << num_samples << " samples:" << std::endl;
    std::cout << "UniformSampler2D: "; sampler_benchmark<UniformSampler2D>(num_samples);
    std::cout << "StratifiedSampler2D: "; sampler_benchmark<StratifiedSampler2D>(num_samples);
    std::cout << "HaltonSampler2D: "; sampler_benchmark<HaltonSampler2D>(num_samples);
    std::cout << "HammersleySampler2D: "; sampler_benchmark<HammersleySampler2D>(num_samples);
    std::cout << "LDSampler2D: "; sampler_benchmark<LDSampler2D>(num_samples);
}
