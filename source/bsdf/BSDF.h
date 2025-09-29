#pragma once
#include "spectrum.hpp"
/*
BSDF: Bidirectional Scattering Distribution Function
BTDF: Bidirectional Transmittance Distribution Function
BRDF: Bidirectional Reflectance Distribution Function
*/
namespace ry {
// BSDF Inline Functions
inline float CosTheta(const vec3& w) { return w.z; }
inline float Cos2Theta(const vec3& w) { return w.z * w.z; }
inline float AbsCosTheta(const vec3& w) { return std::abs(w.z); }
inline float Sin2Theta(const vec3& w) {
    return std::max((float)0, (float)1 - Cos2Theta(w));
}

inline float SinTheta(const vec3& w) { return std::sqrt(Sin2Theta(w)); }

inline float TanTheta(const vec3& w) { return SinTheta(w) / CosTheta(w); }

inline float Tan2Theta(const vec3& w) {
    return Sin2Theta(w) / Cos2Theta(w);
}

inline float CosPhi(const vec3& w) {
    float sinTheta = SinTheta(w);
    return (sinTheta == 0) ? 1 : std::clamp(w.x / sinTheta, -1.f, 1.f);
}

inline float SinPhi(const vec3& w) {
    float sinTheta = SinTheta(w);
    return (sinTheta == 0) ? 0 : std::clamp(w.y / sinTheta, -1.f, 1.f);
}

inline float Cos2Phi(const vec3& w) { return CosPhi(w) * CosPhi(w); }

inline float Sin2Phi(const vec3& w) { return SinPhi(w) * SinPhi(w); }

inline float CosDPhi(const vec3& wa, const vec3& wb) {
    float waxy = wa.x * wa.x + wa.y * wa.y;
    float wbxy = wb.x * wb.x + wb.y * wb.y;
    if (waxy == 0 || wbxy == 0) { return 1; }
    return std::clamp((wa.x * wb.x + wa.y * wb.y) / std::sqrt(waxy * wbxy), -1.f, 1.f);
}

inline vec3 SphericalDirection(float sinTheta, float cosTheta, float phi) {
    return vec3(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
}

inline bool SameHemisphere(const vec3& w, const vec3& wp) {
    return w.z * wp.z > 0;
}

inline vec3 Reflect(const vec3& wo, const vec3& n) {
    return -wo + 2 * dot(wo, n) * n;
}

enum BxDFType {
    BSDF_REFLECTION = 1 << 0,
    BSDF_TRANSMISSION = 1 << 1,
    BSDF_DIFFUSE = 1 << 2,
    BSDF_GLOSSY = 1 << 3,
    BSDF_SPECULAR = 1 << 4,
    BSDF_ALL = BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_SPECULAR | BSDF_REFLECTION |
    BSDF_TRANSMISSION,
};

class BxDF {
public:
    // BxDF Interface
    virtual ~BxDF() {}
    BxDF(BxDFType type) : type(type) {}
    bool MatchesFlags(BxDFType t) const { return (type & t) == type; }
    virtual Spectrum f(const vec3& wo, const vec3& wi) const = 0;
    virtual Spectrum Sample_f(const vec3& wo, vec3* wi,
        const vec2& u, float* pdf,
        BxDFType* sampledType = nullptr) const;
    /*virtual Spectrum rho(const vec3& wo, int nSamples,
        const vec2* samples) const;
    virtual Spectrum rho(int nSamples, const vec2* samples1,
        const vec2* samples2) const;*/
    virtual float Pdf(const vec3& wo, const vec3& wi) const;

    // BxDF Public Data
    const BxDFType type;
};

class LambertianReflection : public BxDF {
public:
    // LambertianReflection Public Methods
    LambertianReflection(const Spectrum& r)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), R(r) {}
    Spectrum f(const vec3& wo, const vec3& wi) const override;
    Spectrum rho(const vec3&, int, const vec2*) const { return R; }
    Spectrum rho(int, const vec2*, const vec2*) const { return R; }

private:
    // LambertianReflection Private Data
    const Spectrum R;
};

class BSDF {
public:
    BSDF(const vec3& sn);
    Spectrum Sample_f(const vec3& woWorld, vec3* wiWorld,
        vec2& u, float* pdf, BxDFType type) const;
    Spectrum f(const vec3& woW, const vec3& wiW,
        BxDFType flags = BSDF_ALL) const;
    void Add(std::unique_ptr<BxDF> b) {
        bxdfs.push_back(std::move(b));
    }
    inline vec3 ToWorld(const vec3& local) const {
        return local.x * t + local.y * b + local.z * n;
    }
    inline vec3 ToLocal(const vec3& world) const {
        return vec3(dot(world, t), dot(world, b), dot(world, n));
    }
private:
    // static int MaxBxDFs = 8;
    std::vector<std::unique_ptr<BxDF>> bxdfs;
    vec3 n, b, t;
};

class Fresnel {
public:
    // Fresnel Interface
    virtual ~Fresnel() {};
    virtual Spectrum Evaluate(float cosI) const = 0;
};

class FresnelNoOp : public Fresnel {
public:
    Spectrum Evaluate(float cosI) const { return Spectrum(1.); }
};

class FresnelDielectric : public Fresnel {
public:
    // FresnelDielectric Public Methods
    Spectrum Evaluate(float cosThetaI) const;
    FresnelDielectric(float etaI, float etaT) : etaI(etaI), etaT(etaT) {}
private:
    float etaI, etaT;
};

class FresnelSchlick : public Fresnel {
public:
    FresnelSchlick(const Spectrum& F0) : F0(F0) {}
    Spectrum Evaluate(float cosThetaI) const override {
        // Schlick : F(theta) = F0 + (1-F0)*(1-cosThetaI)^5
        float factor = powf(1.0f - cosThetaI, 5.0f);
        return F0 + (Spectrum(1.0f) - F0) * factor;
    }
private:
    Spectrum F0;
};

// MicrofacetDistribution Declarations
class MicrofacetDistribution {
public:
    // MicrofacetDistribution Public Methods
    virtual ~MicrofacetDistribution() {};
    virtual float D(const vec3& wh) const = 0;
    virtual float Lambda(const vec3& w) const = 0;
    float G1(const vec3& w) const {
        //    if (Dot(w, wh) * CosTheta(w) < 0.) return 0.;
        return 1 / (1 + Lambda(w));
    }
    virtual float G(const vec3& wo, const vec3& wi) const {
        return 1 / (1 + Lambda(wo) + Lambda(wi));
    }
    virtual vec3 Sample_wh(const vec3& wo, const vec2& u) const = 0;
    float Pdf(const vec3& wo, const vec3& wh) const;

protected:
    // MicrofacetDistribution Protected Methods
    MicrofacetDistribution(bool sampleVisibleArea)
        : sampleVisibleArea(sampleVisibleArea) {}

    // MicrofacetDistribution Protected Data
    const bool sampleVisibleArea;
};

class MicrofacetReflection : public BxDF {
public:
    // MicrofacetReflection Public Methods
    MicrofacetReflection(const Spectrum& R,
        std::shared_ptr<MicrofacetDistribution> distribution, std::shared_ptr<Fresnel> fresnel)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
        R(R),
        distribution(distribution),
        fresnel(fresnel) {}
    Spectrum f(const vec3& wo, const vec3& wi) const;
    Spectrum Sample_f(const vec3& wo, vec3* wi, const vec2& u,
        float* pdf, BxDFType* sampledType) const;
    // float Pdf(const vec3& wo, const vec3& wi) const;

private:
    // MicrofacetReflection Private Data
    const Spectrum R;
    const std::shared_ptr<MicrofacetDistribution> distribution;
    const std::shared_ptr<Fresnel> fresnel;
};

class TrowbridgeReitzDistribution : public MicrofacetDistribution {
public:
    // TrowbridgeReitzDistribution Public Methods
    static inline float RoughnessToAlpha(float roughness) {
        roughness = std::max(roughness, 1e-3f);
        float x = std::log(roughness);
        return 1.62142f + 0.819955f * x + 0.1734f * x * x + 0.0171201f * x * x * x +
            0.000640711f * x * x * x * x;
    }
    TrowbridgeReitzDistribution(float alphax, float alphay,
        bool samplevis = true)
        : MicrofacetDistribution(samplevis),
        alphax(std::max(float(0.001), alphax)),
        alphay(std::max(float(0.001), alphay)) {}
    float D(const vec3& wh) const;
    vec3 Sample_wh(const vec3& wo, const vec2& u)  const override;

private:
    // TrowbridgeReitzDistribution Private Methods
    float Lambda(const vec3& w) const;

    // TrowbridgeReitzDistribution Private Data
    const float alphax, alphay;
};
}