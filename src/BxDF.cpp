#include "BxDF.h"
using namespace std;

Spectrum BSDF::f(const vec3& woW, const vec3& wiW, BxDFType flags) const
{
    return kd / float(M_PI);
}

Spectrum BSDF::Sample_f(const ry::vec3& wo, const ry::vec3& n, ry::vec3& wi_world, float& pdf, Sampler& sampler) const
{
    ry::vec2 u = sampler.Get2D();
    ry::vec3 wi_local = CosineSampleHemisphere(u);
    // 2. 翻转保证和法线同半球
    if (wo.z < 0) wi_local.z *= -1.0f;
    // 3. 转换到世界
    wi_world = LocalToWorld(wi_local, n);
    // 4. 计算pdf
    pdf = Pdf(n, wi_world);
    // 5. 返回BSDF值
    return f(wo, wi_world);
}
