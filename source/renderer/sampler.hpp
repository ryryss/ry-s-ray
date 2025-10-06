#pragma once
#include "pch.h"
namespace ry {
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