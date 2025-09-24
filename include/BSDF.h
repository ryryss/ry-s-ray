#ifndef	BSDF_H
#define BSDF_H
#include "pub.h"

// BSDF Inline Functions
inline float CosTheta(const ry::vec3& w) { return w.z; }
inline float Cos2Theta(const ry::vec3& w) { return w.z * w.z; }
inline float AbsCosTheta(const ry::vec3& w) { return std::abs(w.z); }
inline float Sin2Theta(const ry::vec3& w) {
    return std::max((float)0, (float)1 - Cos2Theta(w));
}

inline float SinTheta(const ry::vec3& w) { return std::sqrt(Sin2Theta(w)); }

inline float TanTheta(const ry::vec3& w) { return SinTheta(w) / CosTheta(w); }

inline float Tan2Theta(const ry::vec3& w) {
    return Sin2Theta(w) / Cos2Theta(w);
}

inline float CosPhi(const ry::vec3& w) {
    float sinTheta = SinTheta(w);
    return (sinTheta == 0) ? 1 : std::clamp(w.x / sinTheta, -1.f, 1.f);
}

inline float SinPhi(const ry::vec3& w) {
    float sinTheta = SinTheta(w);
    return (sinTheta == 0) ? 0 : std::clamp(w.y / sinTheta, -1.f, 1.f);
}

inline float Cos2Phi(const ry::vec3& w) { return CosPhi(w) * CosPhi(w); }

inline float Sin2Phi(const ry::vec3& w) { return SinPhi(w) * SinPhi(w); }

inline float CosDPhi(const ry::vec3& wa, const ry::vec3& wb) {
    float waxy = wa.x * wa.x + wa.y * wa.y;
    float wbxy = wb.x * wb.x + wb.y * wb.y;
    if (waxy == 0 || wbxy == 0) { return 1; }
    return std::clamp((wa.x * wb.x + wa.y * wb.y) / std::sqrt(waxy * wbxy), -1.f, 1.f);
}

inline ry::vec3 SphericalDirection(float sinTheta, float cosTheta, float phi) {
    return ry::vec3(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
}

inline bool SameHemisphere(const ry::vec3& w, const ry::vec3& wp) {
    return w.z * wp.z > 0;
}

inline ry::vec3 Reflect(const ry::vec3& wo, const ry::vec3& n) {
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

class RGBSpectrum {
public:
    RGBSpectrum(float v = 0.f) : c{ v } {}
    RGBSpectrum(ry::vec3 v) : c(v) {}

    RGBSpectrum& operator+=(const RGBSpectrum& c2) {
        c += c2.c;
        return *this;
    }
    RGBSpectrum& operator+=(const ry::vec3& c2) {
        c += c2;
        return *this;
    }
    RGBSpectrum operator*(const float c2) const {
        return RGBSpectrum(c * c2);
    }
    RGBSpectrum operator*(const RGBSpectrum& c2) const {
        return RGBSpectrum(c * c2.c);
    }
    RGBSpectrum& operator*=(const RGBSpectrum& c2) {
        c *= c2.c;
        return *this;
    }
    RGBSpectrum operator+(const RGBSpectrum& c2) const {
        return RGBSpectrum(c + c2.c);
    }
    RGBSpectrum operator/(float c2) const {
        return RGBSpectrum(c / c2);
    }
    bool IsBlack() const {
        for (int i = 0; i < 3; ++i) {
            if (c[i] != 0.) { return false; }
        }
        return true;
    }
    ry::vec3 c;
};
using Spectrum = RGBSpectrum;

class BxDF {
public:
    // BxDF Interface
    virtual ~BxDF() {}
    BxDF(BxDFType type) : type(type) {}
    bool MatchesFlags(BxDFType t) const { return (type & t) == type; }
    virtual Spectrum f(const ry::vec3& wo, const ry::vec3& wi) const = 0;
    virtual Spectrum Sample_f(const ry::vec3& wo, ry::vec3* wi,
        const ry::vec2& u, float* pdf,
        BxDFType* sampledType = nullptr) const;
    /*virtual Spectrum rho(const ry::vec3& wo, int nSamples,
        const ry::vec2* samples) const;
    virtual Spectrum rho(int nSamples, const ry::vec2* samples1,
        const ry::vec2* samples2) const;*/
    virtual float Pdf(const ry::vec3& wo, const ry::vec3& wi) const;

    // BxDF Public Data
    const BxDFType type;
};

class LambertianReflection : public BxDF {
public:
    // LambertianReflection Public Methods
    LambertianReflection(const Spectrum& r)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), R(r) {}
    Spectrum f(const ry::vec3& wo, const ry::vec3& wi) const override;
    Spectrum rho(const ry::vec3&, int, const ry::vec2*) const { return R; }
    Spectrum rho(int, const ry::vec2*, const ry::vec2*) const { return R; }

private:
    // LambertianReflection Private Data
    const Spectrum R;
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

// MicrofacetDistribution Declarations
class MicrofacetDistribution {
public:
    // MicrofacetDistribution Public Methods
    virtual ~MicrofacetDistribution() {};
    virtual float D(const ry::vec3& wh) const = 0;
    virtual float Lambda(const ry::vec3& w) const = 0;
    float G1(const ry::vec3& w) const {
        //    if (Dot(w, wh) * CosTheta(w) < 0.) return 0.;
        return 1 / (1 + Lambda(w));
    }
    virtual float G(const ry::vec3& wo, const ry::vec3& wi) const {
        return 1 / (1 + Lambda(wo) + Lambda(wi));
    }
    virtual ry::vec3 Sample_wh(const ry::vec3& wo, const ry::vec2& u) const = 0;
    float Pdf(const ry::vec3& wo, const ry::vec3& wh) const;

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
    Spectrum f(const ry::vec3& wo, const ry::vec3& wi) const;
    Spectrum Sample_f(const ry::vec3& wo, ry::vec3* wi, const ry::vec2& u,
        float* pdf, BxDFType* sampledType) const;
    // float Pdf(const ry::vec3& wo, const ry::vec3& wi) const;

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
    float D(const ry::vec3& wh) const;
    ry::vec3 Sample_wh(const ry::vec3& wo, const ry::vec2& u)  const override;

private:
    // TrowbridgeReitzDistribution Private Methods
    float Lambda(const ry::vec3& w) const;

    // TrowbridgeReitzDistribution Private Data
    const float alphax, alphay;
};

class BSDF {
public:
    BSDF(const ry::vec3& sn);
    Spectrum BSDF::Sample_f(const ry::vec3& woWorld, ry::vec3* wiWorld,
        ry::vec2& u, float* pdf, BxDFType type) const;
    void Add(std::unique_ptr<BxDF> b) {
        bxdfs.push_back(std::move(b));
    }
    inline ry::vec3 ToWorld(const ry::vec3& local) const {
        return local.x * t + local.y * b + local.z * n;
    }
    inline ry::vec3 ToLocal(const ry::vec3& world) const {
        return ry::vec3(dot(world, t), dot(world, b), dot(world, n));
    }
private:
    // static int MaxBxDFs = 8;
    std::vector<std::unique_ptr<BxDF>> bxdfs;
    ry::vec3 n, b, t;
};

#endif
