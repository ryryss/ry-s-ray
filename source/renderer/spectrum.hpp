#pragma once
#include "pch.h"
namespace ry {
class RGBSpectrum {
public:
    RGBSpectrum(float v = 0.f) : c{ v } {}
    RGBSpectrum(vec3 v) : c(v) {}

    RGBSpectrum& operator+=(const RGBSpectrum& c2) {
        c += c2.c;
        return *this;
    }
    RGBSpectrum& operator+=(const vec3& c2) {
        c += c2;
        return *this;
    }
    RGBSpectrum operator*(const float c2) const {
        return RGBSpectrum(c * c2);
    }
    RGBSpectrum operator*(const RGBSpectrum& c2) const {
        return RGBSpectrum(c * c2.c);
    }
    RGBSpectrum& operator*=(const RGBSpectrum& c2) {
        c *= c2.c;
        return *this;
    }
    RGBSpectrum operator+(const RGBSpectrum& c2) const {
        return RGBSpectrum(c + c2.c);
    }
    RGBSpectrum operator/(float c2) const {
        return RGBSpectrum(c / c2);
    }
    RGBSpectrum operator-(const RGBSpectrum& c2) const {
        return RGBSpectrum(c - c2.c);
    }

    inline bool IsBlack() const {
        for (int i = 0; i < 3; ++i) {
            if (c[i] != 0.) { return false; }
        }
        return true;
    }
    inline float Luminance() const {
        return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
    }
    vec3 c;
};
using Spectrum = RGBSpectrum;
}