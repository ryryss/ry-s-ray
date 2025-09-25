#pragma once
#include "spectrum.hpp"

namespace ry {
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
    Spectrum BSDF::Sample_f(const vec3& woWorld, vec3* wiWorld,
        vec2& u, float* pdf, BxDFType type) const;
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
}
