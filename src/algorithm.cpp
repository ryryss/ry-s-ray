#include "algorithm.h"
using namespace ry;
using namespace alg;
using namespace std;

bool alg::Moller_Trumbore(const vec3& o, const vec3& d, const vec3& a,
    const vec3& b, const vec3& c, float& t, float& g_u, float& g_v)
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
    g_u = u;
    g_v = v;
    return true;
}