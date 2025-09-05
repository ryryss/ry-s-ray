#include "BxDF.h"

Spectrum BSDF::Sample_f(const ry::vec3& wo, ry::vec3* wi, const ry::vec2& u, float* pdf, BxDFType type, BxDFType* sampledType) const
{
    return Spectrum();
}
