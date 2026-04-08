#pragma once
#include "math.hpp"
#include "model.h"
namespace ry {
// Wang hash / PCG
HD inline uint32_t wang_hash(uint32_t seed) {
    seed = (seed ^ 61u) ^ (seed >> 16);
    seed *= 9u;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

// [0, 1)
HD inline float rand01(uint32_t& seed) {
    seed = wang_hash(seed);
    return (seed & 0x00FFFFFF) / float(0x01000000);
}

HD inline mat4 perspective(float fov, float aspect, float zNear, float zFar)
{
    float tanHalfFov = tanf(fov * 0.5f);

    mat4 m;
    m.c0 = vec4(1.0f / (aspect * tanHalfFov), 0, 0, 0);
    m.c1 = vec4(0, 1.0f / tanHalfFov, 0, 0);
    m.c2 = vec4(0, 0, (zFar + zNear) / (zNear - zFar), -1.0f);
    m.c3 = vec4(0, 0, (2.0f * zFar * zNear) / (zNear - zFar), 0);
    return m;
}

HD inline mat4 ortho(float left, float right,
    float bottom, float top,
    float zNear, float zFar)
{
    mat4 m;
    m.c0 = vec4(2.0f / (right - left), 0, 0, 0);
    m.c1 = vec4(0, 2.0f / (top - bottom), 0, 0);
    m.c2 = vec4(0, 0, -2.0f / (zFar - zNear), 0);
    m.c3 = vec4(
        -(right + left) / (right - left),
        -(top + bottom) / (top - bottom),
        -(zFar + zNear) / (zFar - zNear),
        1.0f
    );
    return m;
}
/*
    * @brief Computes the intersection of a ray with a triangle using the Möller–Trumbore algorithm.
    *
    * @param o      [in]  The origin of the ray in world coordinates.
    * @param d      [in]  The direction of the ray (should be normalized).
    * @param a b c  [in]  Triangle.
    * @param t      [out] Distance from ray origin to intersection point along the ray.
    * @param g_u    [out] Barycentric coordinate u at the intersection point.
    * @param g_v    [out] Barycentric coordinate v at the intersection point.
    *
    * @return true if the ray intersects the triangle, false otherwise.
    *
    * @note  - The intersection point can be computed as:
    *          P = orig + t * dir
    *        - The third barycentric coordinate w can be computed as w = 1 - u - v
    *        - The function assumes a right-handed coordinate system.
*/
HD static bool Moller_Trumbore(const vec3& o, const vec3& d, const vec3& a,
    const vec3& b, const vec3& c, float& t, float& gu, float& gv)
{
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
/*
    * @brief Lambertian Shading.
    *
    * @param kd        [in] The diffuse coefficient, or the surface color.
    * @param intensity [in] The intensity of the light source.
    * @param l         [in] The direction of light.
    * @param n         [in] The surface normal.
    *
    * @return The pixel color.
*/
HD static vec4 LambertianShading(const vec4& kd, float intensity, const vec3& l, const vec3& n)
{
    // L = kd * I * max(0, n · l)
    auto lambert_factor = dot(n, l);
    return kd * intensity * glm::max(0.0f, lambert_factor);
}
/*
    * @brief Blinn-Phong Shading.
    *
    * @param kd        [in] The diffuse coefficient, or the surface color.
    * @param ks        [in] The specular coefficient, or the specular color of the surface.
    * @param intensity [in] The intensity of the light source.
    * @param l         [in] The direction of hitpoint to light.
    * @param n         [in] The surface normal.
    * @param v         [in] The direction of hitpoint to eye(camera).
    *
    * @return The pixel color.
*/
HD static vec4 BlinnPhongShading(const vec4& kd, const vec4& ks, float intensity,
    const vec3& l, const vec3& n, const vec3& v)
{
    // L = kd * I * max(0, n · l) + ks * I * max(0, n · h)^p
    uint16_t p = 8;
    auto h = normalize(v + l);
    float f = pow(glm::max(0.0f, dot(n, h)), p);
    return LambertianShading(kd, 1.0/*distance*/, l, n) + ks * intensity * f;
}

HD static Ray RayGeneration(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const Camera& cam)
{
    vec3 o, d;
    // Geometric Method
    // l = -xmag  r = xmag b = -ymag t = ymag
    // u = l + (r − l)(i + 0.5)/nx
    /*float uu = -cam.xmag + 2 * cam.xmag * (x + 0.5) / width;
    float vv = -cam.ymag + 2 * cam.ymag * (y + 0.5) / height;
    if (cam.type == 0) {
        o = cam.e;
        d = normalize(cam.w * cam.znear  + cam.u * uu + cam.v * vv);
    } else {
        o = cam.e + cam.u * uu + cam.v * vv;
        d = cam.w;
    }
    return { o, d };*/

    // Inverse Projection Method
    // display -> NDC -> clip(projection) -> camera(view) -> world
    uint32_t seed = x * 1973u + y * 9277u;
    vec2 ndc = vec2(x + rand01(seed), y + rand01(seed));
    ndc = 2.f * ndc / vec2(width, height) - 1.f;
    vec4 clip(ndc.x, ndc.y, -1, 1);
    vec4 camSpace = cam.clipToCamera * clip;
    camSpace = camSpace / camSpace.w;
    // now in camera coordinates
    vec3 dir = cam.m * camSpace /* - vec3(0, 0, 0) */;
    o = cam.e;
    d = normalize(dir - o); // here the dir represents a point
    return { o, d };
}

HD static bool Intersect(const Triangle* tris, uint32_t triCnt, const Ray& r, Interaction& isect)
{
    isect.hit = -1;
    float t, gu, gv;
    // for (int i = 0; i < triangles.size(); i++) {
    for (int i = 0; i < triCnt; i++) {
        auto& tri = tris[i];
        auto& a = tri.v0;
        auto& b = tri.v1;
        auto& c = tri.v2;
        if (Moller_Trumbore(r.o, r.d, a, b, c, t, gu, gv) &&
            t > isect.tMin && t < isect.tMax) {
            isect.tMax = t;
            isect.bary = { 1 - gu - gv, gu, gv };
            isect.p = r.o + r.d * t;
            isect.hit = i;
        }
    }
    if (isect.hit >= 0) {
        auto& tri = tris[isect.hit];
        isect.normal = normalize(tri.n0 * isect.bary[0]
            + tri.n1 * isect.bary[1] + tri.n2 * isect.bary[2]);
        return true;
    }
    return false;
}
}
