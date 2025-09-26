#include "bsdf.h"
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

BSDF::BSDF(const vec3& sn) : n(sn)
{
    fabs(n.x) > fabs(n.z)?
        t = normalize(vec3(-n.y, n.x, 0)):
        t = normalize(vec3(0, -n.z, n.y));
    b = cross(n, t);
}

Spectrum BSDF::Sample_f(const vec3& woWorld, vec3* wiWorld, vec2& u, float* pdf, BxDFType type) const
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

Spectrum LambertianReflection::f(const vec3& wo, const vec3& wi) const
{
    return R * InvPi;
}

Spectrum BxDF::Sample_f(const vec3& wo, vec3* wi, const vec2& u, float* pdf, BxDFType* sampledType) const
{
    *wi = CosineSampleHemisphere(u);
    if (wo.z < 0) wi->z *= -1;
    *pdf = Pdf(wo, *wi);
    return f(wo, *wi);
}

float BxDF::Pdf(const vec3& wo, const vec3& wi) const
{
    return wo.z * wi.z > 0 ? std::abs(wi.z) * InvPi : 0;
}

Spectrum MicrofacetReflection::f(const vec3& wo, const vec3& wi) const
{
    float cosThetaO = AbsCosTheta(wo), cosThetaI = AbsCosTheta(wi);
    vec3 wh = wi + wo;
    // Handle degenerate cases for microfacet reflection
    if (cosThetaI == 0 || cosThetaO == 0) return Spectrum(0.);
    if (wh.x == 0 && wh.y == 0 && wh.z == 0) return Spectrum(0.);
    wh = normalize(wh);
    // For the Fresnel call, make sure that wh is in the same hemisphere
    // as the surface normal, so that TIR is handled correctly.
    Spectrum F = fresnel->Evaluate(dot(wi, (dot(wh, vec3(0, 0, 1)) < 0.f) ? -wh : wh));
    return R * distribution->D(wh) * distribution->G(wo, wi) * F /
        (4 * cosThetaI * cosThetaO);
}

Spectrum MicrofacetReflection::Sample_f(const vec3& wo, vec3* wi, const vec2& u, float* pdf, BxDFType* sampledType) const
{
    // Sample microfacet orientation $\wh$ and reflected direction $\wi$
    if (wo.z == 0) return 0.;
    vec3 wh = distribution->Sample_wh(wo, u);
    if (dot(wo, wh) < 0) return 0.;   // Should be rare
    *wi = Reflect(wo, wh);
    if (!SameHemisphere(wo, *wi)) return Spectrum(0.f);

    // Compute PDF of _wi_ for microfacet reflection
    *pdf = distribution->Pdf(wo, wh) / (4 * dot(wo, wh));
    return f(wo, *wi);
}

static void TrowbridgeReitzSample11(float cosTheta, float U1, float U2,
    float* slope_x, float* slope_y) {
    // special case (normal incidence)
    if (cosTheta > .9999) {
        float r = sqrt(U1 / (1 - U1));
        float phi = 6.28318530718 * U2;
        *slope_x = r * cos(phi);
        *slope_y = r * sin(phi);
        return;
    }

    float sinTheta =
        std::sqrt(std::max((float)0, (float)1 - cosTheta * cosTheta));
    float tanTheta = sinTheta / cosTheta;
    float a = 1 / tanTheta;
    float G1 = 2 / (1 + std::sqrt(1.f + 1.f / (a * a)));

    // sample slope_x
    float A = 2 * U1 / G1 - 1;
    float tmp = 1.f / (A * A - 1.f);
    if (tmp > 1e10) tmp = 1e10;
    float B = tanTheta;
    float D = std::sqrt(
        std::max(float(B * B * tmp * tmp - (A * A - B * B) * tmp), float(0)));
    float slope_x_1 = B * tmp - D;
    float slope_x_2 = B * tmp + D;
    *slope_x = (A < 0 || slope_x_2 > 1.f / tanTheta) ? slope_x_1 : slope_x_2;

    // sample slope_y
    float S;
    if (U2 > 0.5f) {
        S = 1.f;
        U2 = 2.f * (U2 - .5f);
    } else {
        S = -1.f;
        U2 = 2.f * (.5f - U2);
    }
    float z =
        (U2 * (U2 * (U2 * 0.27385f - 0.73369f) + 0.46341f)) /
        (U2 * (U2 * (U2 * 0.093073f + 0.309420f) - 1.000000f) + 0.597999f);
    *slope_y = S * z * std::sqrt(1.f + *slope_x * *slope_x);
}

static vec3 TrowbridgeReitzSample(const vec3& wi, float alpha_x,
    float alpha_y, float U1, float U2) {
    // 1. stretch wi
    vec3 wiStretched =
        normalize(vec3(alpha_x * wi.x, alpha_y * wi.y, wi.z));

    // 2. simulate P22_{wi}(x_slope, y_slope, 1, 1)
    float slope_x, slope_y;
    TrowbridgeReitzSample11(CosTheta(wiStretched), U1, U2, &slope_x, &slope_y);

    // 3. rotate
    float tmp = CosPhi(wiStretched) * slope_x - SinPhi(wiStretched) * slope_y;
    slope_y = SinPhi(wiStretched) * slope_x + CosPhi(wiStretched) * slope_y;
    slope_x = tmp;

    // 4. unstretch
    slope_x = alpha_x * slope_x;
    slope_y = alpha_y * slope_y;

    // 5. compute normal
    return normalize(vec3(-slope_x, -slope_y, 1.));
}

float TrowbridgeReitzDistribution::D(const vec3& wh) const
{
    float tan2Theta = Tan2Theta(wh);
    if (std::isinf(tan2Theta)) return 0.;
    const float cos4Theta = Cos2Theta(wh) * Cos2Theta(wh);
    float e =
        (Cos2Phi(wh) / (alphax * alphax) + Sin2Phi(wh) / (alphay * alphay)) *
        tan2Theta;
    return 1 / (Pi * alphax * alphay * cos4Theta * (1 + e) * (1 + e));
}

vec3 TrowbridgeReitzDistribution::Sample_wh(const vec3& wo, const vec2& u) const
{
    vec3 wh;
    if (!sampleVisibleArea) {
        float cosTheta = 0, phi = (2 * Pi) * u[1];
        if (alphax == alphay) {
            float tanTheta2 = alphax * alphax * u[0] / (1.0f - u[0]);
            cosTheta = 1 / std::sqrt(1 + tanTheta2);
        } else {
            phi =
                std::atan(alphay / alphax * std::tan(2 * Pi * u[1] + .5f * Pi));
            if (u[1] > .5f) phi += Pi;
            float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
            const float alphax2 = alphax * alphax, alphay2 = alphay * alphay;
            const float alpha2 =
                1 / (cosPhi * cosPhi / alphax2 + sinPhi * sinPhi / alphay2);
            float tanTheta2 = alpha2 * u[0] / (1 - u[0]);
            cosTheta = 1 / std::sqrt(1 + tanTheta2);
        }
        float sinTheta =
            std::sqrt(std::max((float)0., (float)1. - cosTheta * cosTheta));
        wh = SphericalDirection(sinTheta, cosTheta, phi);
        if (!SameHemisphere(wo, wh)) wh = -wh;
    } else {
        bool flip = wo.z < 0;
        wh = TrowbridgeReitzSample(flip ? -wo : wo, alphax, alphay, u[0], u[1]);
        if (flip) wh = -wh;
    }
    return wh;
}

float TrowbridgeReitzDistribution::Lambda(const vec3& w) const
{
    float absTanTheta = std::abs(TanTheta(w));
    if (std::isinf(absTanTheta)) return 0.;
    // Compute _alpha_ for direction _w_
    float alpha =
        std::sqrt(Cos2Phi(w) * alphax * alphax + Sin2Phi(w) * alphay * alphay);
    float alpha2Tan2Theta = (alpha * absTanTheta) * (alpha * absTanTheta);
    return (-1 + std::sqrt(1.f + alpha2Tan2Theta)) / 2;
}

float MicrofacetDistribution::Pdf(const vec3& wo, const vec3& wh) const
{
    if (sampleVisibleArea)
        return D(wh) * G1(wo) * abs(dot(wo, wh)) / AbsCosTheta(wo);
    else
        return D(wh) * AbsCosTheta(wh);
}
