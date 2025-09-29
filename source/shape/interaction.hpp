#pragma once
#include "geometry.hpp"
#include "material.h"
namespace ry{
struct Interaction {
    float tMin;
    float tMax;

    vec3 bary;     // barycentric
    vec3 p;        // hit point
    vec3 normal;   // of hit face

    const Material* mat;
    std::unique_ptr<BSDF> bsdf;
    // now just triangle
    const Triangle* tri;
};
}