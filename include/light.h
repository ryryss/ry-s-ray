#ifndef	LIGHT
#define LIGHT
#include "pub.h"
#include "tracer.h"

namespace ry {
class RGBSpectrum {
public:
    // RGBSpectrum Public Methods
    RGBSpectrum(float v = 0.f) : c{ v } {}
    RGBSpectrum(vec3 v) : c(v) {}
    // protected:
        // CoefficientSpectrum Protected Data
    vec3 c;
};
using Spectrum = RGBSpectrum;

class Light : public Node {
public:
    vec3 Sample_Li(const Interaction& isect, const vec2& u, float pdf)
    {
        // sample a area
        vec3 p01 = mix(p0, p1, u.x);
        vec3 p32 = mix(p3, p2, u.x);
        vec3 p   = mix(p01, p32, u.y);
        return p;
    }

    double intensity = 0.0;

    Light() {};
    Light(const Node& other) : Node(other) {};

    // for path trace, use point light to create a area light
    vec3 p0, p1, p2, p3;
    Spectrum I;
};
}
#endif