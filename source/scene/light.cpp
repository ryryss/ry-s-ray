#include "light.h"
#include "scene.h"
using namespace ry;
using namespace std;
using namespace glm;

Spectrum Light::Sample_Li(const Interaction* isect, vec3* wi, float* pdf) const
{
    // area light
    vec3 samplePoint;
    vec3 lightNormal;
    SamplePoint(samplePoint, lightNormal);
    *wi = samplePoint - isect->p;
    float dist2 = glm::max(dot(*wi, *wi), 0.0f);
    *wi = normalize(*wi);

    Interaction shadowTest;
    shadowTest.tMin = isect->tMin;
    shadowTest.tMax = length(samplePoint - isect->p) - ShadowEpsilon;
    if (model->Intersect(Ray{ isect->p, *wi }, shadowTest)) {
        return Spectrum(0.); // if hit any obeject (include emissive) = shadow
    }

    float cosl = glm::max(0.f, dot(lightNormal, -*wi));
    *pdf = dist2 / (cosl * area); // uniform surface sampling
    Spectrum Le = I.c * emissiveStrength;
    return Le;
}

uint16_t Light::SamplePoint(vec3& pos, vec3& n) const
{
    // sample a point from light, default : area light
    Sampler s;
    uint16_t i = s.GetIntInRange(0, triangles.size() - 1);
    const auto& triangle = model->GetTriangle(triangles[i]);
    
    float u = s.Get1D();
    float v = s.Get1D();
    if (u + v > 1.0f) {
        u = 1 - u;
        v = 1 - v;
    }
    const auto& a = model->GetVertex(triangle->vertIdx[0]);
    const auto& b = model->GetVertex(triangle->vertIdx[1]);
    const auto& c = model->GetVertex(triangle->vertIdx[2]);

    pos = (1 - u - v) * a->pos + u * b->pos + v * c->pos;
    n = (1 - u - v) * a->normal + u * b->normal + v * c->normal; // triangle->normal;
    return i;
}
