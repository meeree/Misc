#pragma once

#include "voxels.h"
#include <random>

class Perlin3D
{
private:
    inline static glm::vec3 RandomGradient(glm::ivec3 const& i)
    {
        // Random float. No precomputed gradients mean this works for any number of grid coordinates
        static float seed1 = rand() % 1000;
        static float seed2 = rand() % 1000;
        static float seed3 = rand() % 1000;
        static float seed4 = rand() % 1000;
        float r1 = 2920.f * sin(seed1 * (i.x * 21942.f + i.y * 171324.f + 8912.f)) * cos(seed2 * (i.x * 23157.f * i.y * 217832.f + 9758.f));
        float r2 = 3371.f * sin(seed3 * (i.y * 289821.f + i.z * 92918.f + 9221.f)) * cos(seed4 * (i.y * 19219.f * i.z * 129102.f + 2819.f));
        return {cos(r2) * cos(r1), cos(r2) * sin(r1), sin(r2)};
    }

    inline static float DotGridGradient(glm::ivec3 const& i, glm::vec3 const& v) 
    {
        // Get gradient from integer coordinates
        glm::vec3 gradient = RandomGradient(i);

        // Compute the dot-product
        return glm::dot(v - glm::vec3(i), gradient);
    }

    inline static float InterpolateLin(float a0, float a1, float w) 
    {
        if (0 > w)
            return a0;
        if (1 < w)
            return a1;
        return (a1 - a0) * w + a0;
    }

    inline static float InterpolateSmooth(float a0, float a1, float w)
    {
        if (0 > w)
            return a0;
        if (1 < w)
            return a1;
        return(a1 - a0) * ((w * (w * 6.0 - 15.0) + 10.0) * w * w * w) + a0;
    }

public:
    Perlin3D() = default;

    // If smooth is set, we use smoothStep intepolate instead of linear.
    float GetPerlin(glm::vec3 p, bool smooth = true)
    {
        auto lerp_fun = smooth ? InterpolateSmooth : InterpolateLin;

        // Determine min and max grid cells. 
        // It is assumed that p is within the grid!
        glm::ivec3 p000 = p;
        glm::ivec3 p111 = p000 + glm::ivec3(1);

        glm::ivec3 p100 = { p111.x, p000.y, p000.z };
        glm::ivec3 p010 = { p000.x, p111.y, p000.z };
        glm::ivec3 p001 = { p000.x, p000.y, p111.z };

        glm::ivec3 p110 = { p111.x, p111.y, p000.z };
        glm::ivec3 p101 = { p111.x, p000.y, p111.z };
        glm::ivec3 p011 = { p000.x, p111.y, p111.z };

        glm::vec3 interp{p - glm::vec3(p000)};

        // Z = 0
        float n0 = DotGridGradient(p000, p);
        float n1 = DotGridGradient(p100, p);
        float ix0 = lerp_fun(n0, n1, interp.x);

        n0 = DotGridGradient(p010, p);
        n1 = DotGridGradient(p110, p);
        float ix1 = lerp_fun(n0, n1, interp.x);

        // Interpolate Y to get value for Z = 0 face.
        float iy0 = lerp_fun(ix0, ix1, interp.y);

        // Z = 1
        n0 = DotGridGradient(p001, p);
        n1 = DotGridGradient(p101, p);
        ix0 = lerp_fun(n0, n1, interp.x);

        n0 = DotGridGradient(p011, p);
        n1 = DotGridGradient(p111, p);
        ix1 = lerp_fun(n0, n1, interp.x);

        // Interpolate Y to get value for Z = 1 face.
        float iy1 = lerp_fun(ix0, ix1, interp.y);

        // Interpolate Z between Z = 0 and Z = 1 face.
        return lerp_fun(iy0, iy1, interp.z);
    }

    // Test on an NxNxN grid with K samples per k gridcell.
    std::vector<std::vector<std::vector<float>>> Test(int N, int K)
    {
        std::vector<std::vector<std::vector<float>>> vals(N * K, std::vector<std::vector<float>>(N * K, std::vector<float>(N * K)));
        float step = 1.0f / K;
        for (int n0 = 0; n0 < N; ++n0)
            for (int n1 = 0; n1 < N; ++n1)
                for (int n2 = 0; n2 < N; ++n2)
                    for (int k0 = 0; k0 < K; ++k0)
                        for (int k1 = 0; k1 < K; ++k1)
                            for (int k2 = 0; k2 < K; ++k2)
                                vals[n0 * K + k0][n1 * K + k1][n2 * K + k2] = GetPerlin({ n0 + (k0+0.5) * step, n1 + (k1+0.5) * step,  n2 + (k2+0.5) * step });

        return vals;
    }
};

// This is just for testing out my perlin noise implementation. It is not used in the game.
class Perlin2D
{
private:
    inline static glm::vec2 RandomGradient(glm::ivec2 const& xy)
    {
        // Random float. No precomputed gradients mean this works for any number of grid coordinates
        float random = 2920.f * sin(xy.x * 21942.f + xy.y * 171324.f + 8912.f) * cos(xy.x * 23157.f * xy.y * 217832.f + 9758.f);
        return { cos(random), sin(random) };
    }

    inline static float DotGridGradient(glm::ivec2 const& i, glm::vec2 const& v) 
    {
        // Get gradient from integer coordinates
        glm::vec2 gradient = RandomGradient(i);

        // Compute the dot-product
        return glm::dot(v - glm::vec2(i), gradient);
    }

    inline static float InterpolateLin(float a0, float a1, float w) 
    {
        if (0 > w)
            return a0;
        if (1 < w)
            return a1;
        return (a1 - a0) * w + a0;
    }

    inline static float InterpolateSmooth(float a0, float a1, float w)
    {
        if (0 > w)
            return a0;
        if (1 < w)
            return a1;
        return(a1 - a0) * ((w * (w * 6.0 - 15.0) + 10.0) * w * w * w) + a0;
    }

public:
    Perlin2D() = default;

    // If smooth is set, we use smoothStep intepolate instead of linear.
    float GetPerlin(glm::vec2 p, bool smooth=true)
    {
        auto lerp_fun = smooth ? InterpolateSmooth : InterpolateLin;

        // Determine min and max grid cells. 
        // It is assumed that p is within the grid!
        glm::ivec2 p00 = p;
        glm::ivec2 p11 = p00 + glm::ivec2(1);
        glm::ivec2 p01 = { p00.x, p11.y };
        glm::ivec2 p10 = { p11.x, p00.y };

        float n0 = DotGridGradient(p00, p);
        float n1 = DotGridGradient(p10, p);
        float ix0 = lerp_fun(n0, n1, p.x - p00.x);

        n0 = DotGridGradient(p01, p);
        n1 = DotGridGradient(p11, p);
        float ix1 = lerp_fun(n0, n1, p.x - p00.x);

        return lerp_fun(ix0, ix1, p.y - p00.y);
    }

    // Test on an NxN grid with K samples per k gridcell.
    std::vector<std::vector<float>> Test(int N, int K)
    {
        std::vector<std::vector<float>> vals(N * K, std::vector<float>(N * K));
        float step = 1.0f / K;
        for (int n0 = 0; n0 < N; ++n0)
            for (int n1 = 0; n1 < N; ++n1)
                for (int k0 = 0; k0 < K; ++k0)
                    for (int k1 = 0; k1 < K; ++k1)
                        vals[n0 * K + k0][n1 * K + k1] = GetPerlin({ n0 + (k0+0.5) * step, n1 + (k1+0.5) * step });

        return vals;
    }
};
