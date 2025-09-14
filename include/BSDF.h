#ifndef	BSDF_H
#define BSDF_H
#include "pub.h"

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
    RGBSpectrum& operator+(const RGBSpectrum& c2) const {
        return RGBSpectrum(c + c2.c);
    }
    RGBSpectrum& operator/(float c2) const {
        return RGBSpectrum(c / c2);
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
