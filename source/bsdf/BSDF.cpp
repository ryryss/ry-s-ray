#include "BSDF.h"
using namespace ry;
using namespace std;
vec2 ConcentricSampleDisk(const vec2& u) {
    // Map uniform random numbers to $[-1,1]^2$
    vec2 uOffset = 2.f * u - vec2(1, 1);

    // Handle degeneracy at the origin
    if (uOffset.x == 0 && uOffset.y == 0) return vec2(0, 0);

    // Apply concentric mapping to point
    float theta, r;
    if (std::abs(uOffset.x) > std::abs(uOffset.y)) {
        r = uOffset.x;
        theta = PiOver4 * (uOffset.y / uOffset.x);
    } else {
        r = uOffset.y;
        theta = PiOver2 - PiOver4 * (uOffset.x / uOffset.y);
    }
    return r * vec2(std::cos(theta), std::sin(theta));
}

inline vec3 CosineSampleHemisphere(const vec2& u) {
    vec2 d = ConcentricSampleDisk(u);
    float z = std::sqrt(std::max((float)0, 1 - d.x * d.x - d.y * d.y));
    return vec3(d.x, d.y, z);
}

BSDF::BSDF(const ry::vec3& sn) : n(sn)
{
    fabs(n.x) > fabs(n.z)?
        t = normalize(vec3(-n.y, n.x, 0)):
        t = normalize(vec3(0, -n.z, n.y));
    b = cross(n, t);
}

Spectrum BSDF::Sample_f(const ry::vec3& woWorld, ry::vec3* wiWorld, ry::vec2& u, float* pdf, BxDFType type) const
{    
    // Cosine-sample the hemisphere, flipping the direction if necessary
    vec3 wi, wo = ToLocal(woWorld);
    Spectrum f;
    for (auto& bxdf : bxdfs) {
        f += bxdf->Sample_f(wo, &wi, u, pdf);
    }
    *wiWorld = ToWorld(wi);
    return f;
}

Spectrum LambertianReflection::f(const ry::vec3& wo, const ry::vec3& wi) const
{
    return R * InvPi;
}

Spectrum BxDF::Sample_f(const ry::vec3& wo, ry::vec3* wi, const ry::vec2& u, float* pdf, BxDFType* sampledType) const
{
    *wi = CosineSampleHemisphere(u);
    if (wo.z < 0) wi->z *= -1;
    *pdf = Pdf(wo, *wi);
    return f(wo, *wi);
}

float BxDF::Pdf(const ry::vec3& wo, const ry::vec3& wi) const
{
    return wo.z * wi.z > 0 ? std::abs(wi.z) * InvPi : 0;
}
