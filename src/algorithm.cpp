#include "algorithm.h"
using namespace ry;
using namespace alg;
using namespace glm;
using namespace std;

bool alg::Moller_Trumbore(const vec3& o, const vec3& d, const vec3& a,
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

vec3 alg::LambertianShading(const vec3& kd, float intensity, const vec3& l, const vec3& n)
{
    // L = kd * I * max(0, n ¡¤ l)
    auto lambert_factor = dot(n, l);
    return kd * intensity * glm::max(0.0f, lambert_factor);
}

vec3 alg::BlinnPhongShading(const vec3& kd, const vec3& ks, float intensity,
    const vec3& l, const vec3& n, const vec3& v)
{
    // L = kd * I * max(0, n ¡¤ l) + ks * I * max(0, n ¡¤ h)^p
    uint16_t p = 32;
    auto h = normalize(v + l);
    float f = pow(glm::max(0.0f, dot(n, h)), p);
    return LambertianShading(kd, 1.0/*lgt.intensity * distance*/, l, n) + ks * intensity * f;
}