#include "light.h"
#include "scene.h"
using namespace ry;
using namespace std;
using namespace glm;

Spectrum Light::Sample_Li(const Scene* scene, const ry::Interaction* isect, ry::vec3& wi) const
{
    Spectrum Li(0.);
    const auto& a = scene->GetVertex(isect->tri->vertIdx[0]);
    const auto& b = scene->GetVertex(isect->tri->vertIdx[1]);
    const auto& c = scene->GetVertex(isect->tri->vertIdx[2]);
    // sample a point from light, default : area light
    Sampler s;
    float u = s.Get1D();
    float v = s.Get1D();
    if (u + v > 1.0f) {
        u = 1 - u;
        v = 1 - v;
    }
    vec3 samplePoint = (1 - u - v) * a->pos + u * b->pos + v * c->pos;
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
    vec2 uv = isect->bary[0] * a->uv + isect->bary[1] * b->uv + isect->bary[2] * c->uv;
    Spectrum kd = mat->GetAlbedo(uv);

    vec3 nl = isect->tri->normal;
    float cosl = glm::max(0.f, dot(nl, -wi));
    float cosp = glm::max(0.f, dot(isect->normal, wi));
    float pdf = 1.0f / area;
    Spectrum Le = I.c * emissiveStrength;// / (Pi * lgt.area);
    Spectrum f = kd / Pi;
    Li += f * Le * cosp * cosl / (dist2 * pdf);
    return Li;
}