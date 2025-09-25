#include "light.h"
#include "scene.h"
using namespace ry;
using namespace std;
using namespace glm;

Spectrum Light::Sample_Li(const Scene* scene, const Interaction* isect, vec3& wi) const
{
    Spectrum Li(0.);
    vec3 samplePoint;
    vec3 lightNormal;
    SamplePoint(scene, samplePoint, lightNormal);
    wi = samplePoint - isect->p;
    float dist2 = glm::max(dot(wi, wi), 0.0f);
    wi = normalize(wi);

    Interaction shadowTest;
    shadowTest.tMin = isect->tMin;
    shadowTest.tMax = length(samplePoint - isect->p);
    if (scene->Intersect(Ray{ isect->p, wi }, shadowTest)) {
        return Spectrum(0.); // if hit any obeject (include emissive) = shadow
    }

    const Material* mat = isect->mat;
    const auto& a = scene->GetVertex(isect->tri->vertIdx[0]);
    const auto& b = scene->GetVertex(isect->tri->vertIdx[1]);
    const auto& c = scene->GetVertex(isect->tri->vertIdx[2]);
    vec2 uv = isect->bary[0] * a->uv + isect->bary[1] * b->uv + isect->bary[2] * c->uv;
    Spectrum kd = mat->GetAlbedo(uv);

    float cosl = glm::max(0.f, dot(lightNormal, -wi));
    float cosp = glm::max(0.f, dot(isect->normal, wi));
    float pdf = 1.0f / area; // uniform surface sampling
    Spectrum Le = I.c * emissiveStrength;// / (Pi * lgt.area);
    Spectrum f = kd / Pi;
    Li += f * Le * cosp * cosl / (dist2 * pdf);
    return Li;
}

uint16_t Light::SamplePoint(const Scene* scene, vec3& pos, vec3& n) const
{
    // sample a point from light, default : area light
    Sampler s;
    uint16_t i = s.GetIntInRange(0, triangles.size() - 1);
    auto triangle = scene->GetTriangle(s.GetIntInRange(0, triangles.size() - 1));
    
    float u = s.Get1D();
    float v = s.Get1D();
    if (u + v > 1.0f) {
        u = 1 - u;
        v = 1 - v;
    }
    const auto& a = scene->GetVertex(triangle->vertIdx[0]);
    const auto& b = scene->GetVertex(triangle->vertIdx[1]);
    const auto& c = scene->GetVertex(triangle->vertIdx[2]);

    pos = (1 - u - v) * a->pos + u * b->pos + v * c->pos;
    n = triangle->normal;
    return i;
}
