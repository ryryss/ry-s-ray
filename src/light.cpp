#include "light.h"
#include "loder.h"

using namespace ry;
using namespace std;
using namespace glm;

Spectrum Light::Sample_Li(const Sampler& s, const Interaction* isect, vec3& wi)
{
    Spectrum Li(0.);
    auto& model = Loader::GetInstance();
    // sample light
    int lgtTriIdx = glm::linearRand(0, (int)tris.size() - 1);
    auto tri = tris[lgtTriIdx];
    auto a = tri->vts[0];
    auto b = tri->vts[1];
    auto c = tri->vts[2];

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
    vec3 nt = normalize(isect->bary[0] * a->normal + isect->bary[1] * b->normal + isect->bary[2] * c->normal);
    Interaction isect2;
    if (isect2.Intersect(Ray{ isect->p + nt * ShadowEpsilon, wi }, model.GetTriangles(),
        length(samplePoint - isect->p), model.GetCam().zfar)) {
        return Spectrum(0.); // if hit any obeject (include emissive) = shadow
    }

    const Material& mat = model.GetMaterial(tri->material);
    vec3 kd = vec3(mat.pbrMetallicRoughness.baseColorFactor[0],
        mat.pbrMetallicRoughness.baseColorFactor[1], mat.pbrMetallicRoughness.baseColorFactor[2]);

    vec3 nl = tri->normal;
    float cosl = glm::max(0.f, dot(nl, -wi));
    float cosp = glm::max(0.f, dot(nt, wi));
    float pdf = 1.0f / area;
    vec3 Le = I.c * emissiveStrength;// / (Pi * lgt.area);
    vec3 f = kd / Pi;
    Li += f * Le * cosp * cosl / (dist2 * pdf);
    return Li;
}