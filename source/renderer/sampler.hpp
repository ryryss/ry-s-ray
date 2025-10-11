#pragma once
#include "pch.h"
namespace ry {
template<typename T, typename FetchFunc>
T SampleNearest(float u, float v, int width, int height, FetchFunc fetch)
{
    float fx = u * (width - 1);
    float fy = v * (height - 1);

    int x = int(fx + 0.5f);
    int y = int(fy + 0.5f);

    x = std::clamp(x, 0, width - 1);
    y = std::clamp(y, 0, height - 1);

    return fetch(x, y);
}

template<typename T, typename FetchFunc>
T SampleBilinear(float u, float v, int width, int height, FetchFunc fetch)
{
    float fx = u * (width - 1);
    float fy = v * (height - 1);

    int x0 = int(fx);
    int y0 = int(fy);
    int x1 = std::min(x0 + 1, width - 1);
    int y1 = std::min(y0 + 1, height - 1);

    float dx = fx - x0;
    float dy = fy - y0;

    T c00 = fetch(x0, y0);
    T c10 = fetch(x1, y0);
    T c01 = fetch(x0, y1);
    T c11 = fetch(x1, y1);

    T c0 = c00 * (1.0f - dx) + c10 * dx;
    T c1 = c01 * (1.0f - dx) + c11 * dx;
    return c0 * (1.0f - dy) + c1 * dy;
}

class Sampler {
private:
    mutable std::mt19937 generator;
    mutable std::uniform_real_distribution<float> distF;
public:
    Sampler() : distF(0.0f, 1.0f) {
        std::random_device rd;
        generator.seed(rd());
    }

    inline vec2 Get2D() const {
        return vec2(Get1D(), Get1D());
    }

    inline float Get1D() const {
        return distF(generator);
    }

    inline int GetIntInRange(int min, int max) const {
        if (min > max) {
            std::swap(min, max);
        }
        std::uniform_int_distribution<int> disI(min, max);
        return disI(generator);
    }
};
}