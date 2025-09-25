#pragma once
#include "pch.h"
namespace ry {
class RGBSpectrum {
public:
    RGBSpectrum(float v = 0.f) : c{ v } {}
    RGBSpectrum(ry::vec3 v) : c(v) {}

    RGBSpectrum& operator+=(const RGBSpectrum& c2) {
        c += c2.c;
        return *this;
    }
    RGBSpectrum& operator+=(const ry::vec3& c2) {
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
    ry::vec3 c;
};
using Spectrum = RGBSpectrum;
}