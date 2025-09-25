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

    Material* mat;
    std::unique_ptr<BSDF> b;
    // now just triangle
    std::array<const Vertex*, 3> vts;
    const Triangle* tri;
#ifdef DEBUG
    std::vector<uint64_t> record;
#endif
};
}