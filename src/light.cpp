#include "light.h"
#include "loader.h"

using namespace ry;
using namespace std;
using namespace glm;

Spectrum Light::Sample_Li(const Sampler& s, const Interaction* isect, vec3& wi)
{
    Spectrum Li(0.);
    auto& model = Loader::GetInstance();
    auto& ts = model.GetTriangles();
    // sample light
    int lgtTriIdx = s.GetIntInRange(0, tris.size() - 1);
    auto tri = ts[tris[lgtTriIdx]];
    auto vts = model.GetTriVts(tri);

    float u = s.Get1D();
    float v = s.Get1D();
    if (u + v > 1.0f) {
        u = 1 - u;
        v = 1 - v;
    }
    vec3 samplePoint = (1 - u - v) * vts[0]->pos + u * vts[1]->pos + v * vts[2]->pos;
    wi = samplePoint - isect->p;
    float dist2 = glm::max(dot(wi, wi), 0.0f);
    wi = normalize(wi);

    Interaction shadowTest;
    if (shadowTest.Intersect(Ray{ isect->p, wi }, model.GetCam().znear, length(samplePoint - isect->p) - ShadowEpsilon)) {
        return Spectrum(0.); // if hit any obeject (include emissive) = shadow
    }

    const Material& mat = model.GetMaterial(isect->tri->material);
    vec3 kd = mat.GetAlbedo(*isect).c;

    vec3 nl = tri.normal;
    float cosl = glm::max(0.f, dot(nl, -wi));
    float cosp = glm::max(0.f, dot(isect->normal, wi));
    float pdf = 1.0f / area;
    vec3 Le = I.c * emissiveStrength;// / (Pi * lgt.area);
    vec3 f = kd / Pi;
    Li += f * Le * cosp * cosl / (dist2 * pdf);
    return Li;
}