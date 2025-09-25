#pragma once
#include "pch.h"
namespace ry{
static bool Moller_Trumbore(const vec3& o, const vec3& d, const vec3& a,
    const vec3& b, const vec3& c, float& t, float& gu, float& gv)
{
    // tri base vec
    vec3 e1 = b - a;
    vec3 e2 = c - a;

    vec3 s1 = cross(d, e2);
    float det = dot(e1, s1);
    if (fabs(det) <= 1e-8) {
        return false;
    }

    vec3 s = o - a;
    float ted = 1 / det;
    float u = dot(s, s1) * ted;
    if (u < 0.0f || u > 1.0f) { // use center of gravity coordinates to judge
        return false;
    }

    vec3 s2 = cross(s, e1);
    float v = dot(d, s2) * ted;
    if (v < 0.0f || v + u > 1.0f) {
        return false;
    }

    t = dot(e2, s2) * ted;
    gu = u;
    gv = v;
    return true;
}

struct Vertex {
    vec3 pos;
    vec4 color = vec4(1.0);
    vec3 normal;
    vec2 uv;
    int i;
};

struct Triangle {
    vec4 color;
    vec3 c; // centroid
    int material;
    uint32_t vertIdx[3];
    vec3 normal;
    int i;
};

struct Ray {
    vec3 o;
    vec3 d;
    vec3 dInv;
    int sign[3];
    Ray(const vec3& origin, const vec3& dir) : o(origin), d(dir) {
        dInv = vec3(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);
    }
};
}