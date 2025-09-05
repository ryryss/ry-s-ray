#ifndef BXDF
#define BXDF
#include "pub.h"
using namespace ry;
// reference from https://github.com/mmp/pbrt-v3
// BSDF Declarations
enum BxDFType {
    BSDF_REFLECTION = 1 << 0,
    BSDF_TRANSMISSION = 1 << 1,
    BSDF_DIFFUSE = 1 << 2,
    BSDF_GLOSSY = 1 << 3,
    BSDF_SPECULAR = 1 << 4,
    BSDF_ALL = BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_SPECULAR | BSDF_REFLECTION |
    BSDF_TRANSMISSION,
};

// BxDF Declarations
class BxDF {
public:
    // BxDF Interface
    virtual ~BxDF() {}
    BxDF(BxDFType type) : type(type) {}
    bool MatchesFlags(BxDFType t) const { return (type & t) == type; }
    virtual Spectrum f(const ry::vec3& wo, const ry::vec3& wi) const = 0;
    virtual Spectrum Sample_f(const ry::vec3& wo, ry::vec3* wi,
        const ry::vec2& sample, float* pdf,
        BxDFType* sampledType = nullptr) const;
    virtual Spectrum rho(const ry::vec3& wo, int nSamples,
        const ry::vec2* samples) const;
    virtual Spectrum rho(int nSamples, const ry::vec2* samples1,
        const ry::vec3* samples2) const;
    virtual float Pdf(const ry::vec3& wo, const ry::vec3& wi) const;
    virtual std::string ToString() const = 0;

    // BxDF Public Data
    const BxDFType type;
};

class BSDF {
public:
    // BSDF Public Methods
    BSDF(BxDFType type) : type(type) {}
    Spectrum f(const ry::vec3& woW, const ry::vec3& wiW,
        BxDFType flags = BSDF_ALL) const;
    Spectrum rho(int nSamples, const ry::vec2* samples1, const ry::vec2* samples2,
        BxDFType flags = BSDF_ALL) const;
    Spectrum rho(const ry::vec3& wo, int nSamples, const ry::vec2* samples,
        BxDFType flags = BSDF_ALL) const;
    Spectrum Sample_f(const ry::vec3& wo, ry::vec3* wi, const ry::vec2& u,
        float* pdf, BxDFType type = BSDF_ALL,
        BxDFType* sampledType = nullptr) const;
    float Pdf(const ry::vec3& wo, const ry::vec3& wi,
        BxDFType flags = BSDF_ALL) const;
    std::string ToString() const;

    const BxDFType type;
private:
    // BSDF Private Methods
    ~BSDF() {}
};
#endif