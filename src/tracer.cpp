#include "tracer.h"
#include "algorithm.h"
#include "task.h"

using namespace std;
using namespace ry;
using namespace glm;
using namespace alg;
uint8_t Tracer::maxDeep = 4;

// Ambient Shading
vec4 A = vec4(0.051, 0.051, 0.051, 1.0) * 1.0f;

void Tracer::ProcessCamera()
{
    auto& cam = model->GetCam();
    // use cam aspect ratio TODO: move function to loder
    if (cam.type == "perspective") {
        cam.ymag = cam.znear * tan(cam.yfov / 2);
        cam.xmag = cam.ymag * cam.aspectRatio;
    } else {
        cam.xmag = cam.ymag * scr.w / scr.h;
    }
    tMin = cam.znear;
    tMax = cam.zfar;
}

void Tracer::Excute(const Screen& s)
{
    if (s.h <= 0 && s.w <= 0) {
        // throw ("");
    }
    else if (scr.h == s.h && scr.w == s.w) {
        
    } else {
        scr = s;
        pixels.clear();
        pixels.resize(s.h * s.w);
        ProcessCamera();
    }
    Calculate();
}

void Tracer::Calculate()
{
    auto& t = Task::GetInstance();
    auto w_cnt = t.WokerCnt() - 1;
    for (auto i = 0; i < w_cnt; i ++) {
        t.Add([this, i, &t, h = scr.h / w_cnt]() {
            // use the number of pixels on the y-axis to parallel cal
            for (uint16_t y = i * h; y < (i + 1) * h; y++) {
                for (uint16_t x = 0; x < scr.w; x++) {
                    Ray r = RayGeneration(x, y);
                    pixels[y * scr.w + x] = vec4(RayCompute(x, y).c, 1.0);
                }
            }
        });
    }
    t.AsynExcute();

    if (auto mod = scr.h % w_cnt; mod) {
        for (uint16_t y = scr.h / w_cnt * w_cnt; y < scr.h; y++) {
            for (uint16_t x = 0; x < scr.w; x++) {
                pixels[y * scr.w + x] = vec4(RayCompute(x, y).c, 1.0);
            }
        }
    }
    while (!t.Free());
    // below is the serial version
    // l = -xmag  r = xmag b = -ymag t = ymag
    // u = l + (r − l)(i + 0.5)/nx
    /*for (uint16_t y = 0; y < scr.h; y++) {
        for (uint16_t x = 0; x < scr.w; x++) {
            RayCompute(x, y);
        }
    }*/
}
Ray Tracer::RayGeneration(uint32_t x, uint32_t y)
{
    vec3 o, d;
    auto& cam = model->GetCam();
    float uu = -cam.xmag + 2 * cam.xmag * (x + 0.5) / scr.w;
    float vv = -cam.ymag + 2 * cam.ymag * (y + 0.5) / scr.h;
    if (cam.type == "perspective") {
        o = cam.e;
        d = normalize(cam.znear * cam.w + cam.u * uu + cam.v * vv);
    } else {
        o = cam.e + cam.u * uu + cam.v * vv;
        d = cam.w;
    }
    return { o, d };
}

ry::Spectrum Tracer::RayCompute(uint32_t x, uint32_t y)
{
    Ray r = RayGeneration(x, y);
    Spectrum Lo(0.);

    Interaction isect;
    if (!Intersect(r, tMax, &isect)) {
        return A;
    }

    Lo += Li(r, isect, 0);
    return Lo;
}

// need input 2 random float ∈ [0,1]^2
vec3 CosineSampleHemisphere(const vec2& u) {
    float r = sqrt(u.x);
    float theta = 2.0f * M_PI * u.y;

    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(glm::max(0.0f, 1.0f - u.x));

    return vec3(x, y, z); // local value
}

void CoordinateSystem(const vec3& n, vec3& t, vec3& b) {
    if (fabs(n.x) > fabs(n.z))
        t = normalize(vec3(-n.y, n.x, 0));
    else
        t = normalize(vec3(0, -n.z, n.y));
    b = cross(n, t);
}

vec3 ToWorld(const vec3& local, const vec3& n) {
    vec3 t, b;
    CoordinateSystem(n, t, b);
    return local.x * t + local.y * b + local.z * n;
}

vec3 SampleTriangle(const vec3& a, const vec3& b, const vec3& c){
    Sampler sampler;
    float u = sampler.Get1D();
    float v = sampler.Get1D();
    if (u + v > 1.0f) {
        u = 1 - u;
        v = 1 - v;
    }
    return (1 - u - v) * a + u * b + v * c;
}

int GetRandom(int i) {
    return rand() % i + 1;
}

Spectrum Tracer::EstimateDirect(const Interaction& isect)
{
    Spectrum Lo(0.);
    // sample light
    const auto& vs = model->GetVertices();
    auto& lgt = model->GetLgt();
    int lgtTriIdx = glm::linearRand(0, (int)lgt.tris.size() - 1);
    auto lt = lgt.tris[lgtTriIdx];
    auto& v0 = vs[lt.idx[0]].pos;
    auto& v1 = vs[lt.idx[1]].pos;
    auto& v2 = vs[lt.idx[2]].pos;
    auto sp = SampleTriangle(v0, v1, v2);
    vec3 wi = sp - isect.p;
    float dist2 = glm::max(dot(wi, wi), 0.0f);
    wi = normalize(wi);

    isect.tri;
    vec3 nt = model->GetTriNormalizeByBary(isect.tri.i, isect.tri.bary);
    Interaction isect2;
    if (Intersect(Ray{ isect.p + nt * 1e-5f, wi }, length(sp - isect.p), &isect2)) {
        const Material& mat = model->GetMaterial(isect2.tri.material);
        if (!model->isEmissive(isect2.tri.material)) {
            return Spectrum(0.); // if hit any obeject without emissive = shadow
        }
    }

    const Material& mat = model->GetMaterial(isect.tri.material);
    vec3 kd = vec3(mat.pbrMetallicRoughness.baseColorFactor[0],
        mat.pbrMetallicRoughness.baseColorFactor[1], mat.pbrMetallicRoughness.baseColorFactor[2]);

    vec3 nl = model->GetTriNormalize(lgt.tris[lgtTriIdx].i);
    float cosl = glm::max(0.f, dot(nl, -wi));
    float cosp = glm::max(0.f, dot(nt, wi));
    float pdf = 1.0f / lgt.area;
    vec3 Le = lgt.I.c * lgt.emissiveStrength * 0.5f;// / (M_PI * lgt.area);
    vec3 f = kd / M_PI;// material.BRDF(wo, wi);
    Lo += f * Le * cosp * cosl / (dist2 * pdf);
    return Lo;
}

Spectrum Tracer::Li(const Ray& r, const Interaction& isect, int depth)
{
    Spectrum Lo(0.);
    if (depth >= maxDeep) {
        return Lo;
    }
    if (depth == 0) {
        // process self-luminous objects
    }
    Lo += EstimateDirect(isect);
    
    vec3 nt = model->GetTriNormalizeByBary(isect.tri.i, isect.tri.bary);
    const Material& mat = model->GetMaterial(isect.tri.material);
    Spectrum kd;
    if (model->isEmissive(isect.tri.material)) {
        auto& lgt = model->GetLgt();
        return Spectrum(lgt.I.c * lgt.emissiveStrength * 0.5f);
    } else {
        kd = Spectrum(vec3(mat.pbrMetallicRoughness.baseColorFactor[0],
            mat.pbrMetallicRoughness.baseColorFactor[1], mat.pbrMetallicRoughness.baseColorFactor[2]));
    }

    int bounces = 6;
    Spectrum Ld;
    for (int i = 0; i < bounces; ++i) {
        Sampler s;
        vec3 wi = ToWorld(CosineSampleHemisphere(s.Get2D())/*vec3(1.0f, 0.0f, 0.0f)*/, nt);
        Interaction isect2;
        if (!Intersect(Ray{ isect.p + nt * 1e-5f, wi }, tMax, &isect2)) {
            Ld.c += A;
            continue;
        } else if (model->isEmissive(isect2.tri.material)) {
            auto & lgt = model->GetLgt();
            Ld.c += lgt.I.c * lgt.emissiveStrength * 0.5f;
            continue;
        }
        vec3 n2 = model->GetTriNormalizeByBary(isect2.tri.i, isect2.tri.bary);
        Spectrum Le = Li(Ray{ isect2.p, -wi}, isect2, depth + 1);
        Spectrum f = kd.c / float(M_PI);
        float cosTheta = glm::max(dot(nt, wi), 0.f);
        float pdf = cosTheta / 2 / M_PI;
        Ld += f.c * Le.c * cosTheta / pdf;
    }
    Lo += (Ld / (float)bounces);
    return Lo;
}

vec4 Tracer::SampleTexture(const vec3& bary, const vec2& uv0, const vec2& uv1, const vec2& uv2)
{
    vec2 uv = bary[0] * uv0 + bary[1] * uv1 + bary[2] * uv2;
    const auto& image = model->GetTexTureImg();
    const auto* pixel = image.image.data();
    if (pixel == nullptr) {
        return vec4(1.0, 0, 0, 1.0);
    }
    uint32_t x = uv[0] * (image.width - 1);
    uint32_t y = uv[1] * (image.height - 1); // no need reverse
    // int((1.0f - v) * (image.height - 1));
    int idx = (y * image.width + x) * image.component;

    return { pixel[idx + 0] / 255.0f,
             pixel[idx + 1] / 255.0f,
             pixel[idx + 2] / 255.0f,
            (image.component == 4) ? pixel[idx + 3] / 255.0f : 1.0f };
}

bool Tracer::Intersect(const Ray& r, const float dis, Interaction* isect)
{
    bool hit = false;
    float t, gu, gv;
    float mt = tMax;
    const auto& ts = model->GetTriangles();
    const auto& vs = model->GetVertices();
    for (auto& it : ts) {
        const vec3& a = vs[it.idx[0]].pos;
        const vec3& b = vs[it.idx[1]].pos;
        const vec3& c = vs[it.idx[2]].pos;
        if (Moller_Trumbore(r.o, r.d, a, b, c, t, gu, gv) &&
            t > tMin && t < dis && t < mt && t < tMax) {
            mt = t;
            *isect = Interaction(it, mt);
            isect->tri.bary = { 1 - gu - gv, gu, gv };
            isect->p = r.o + t * r.d;
            hit = true;
        }
    }
    return hit;
}

bool Tracer::IntersectAny(const Ray& r, const float dis, Interaction* isect)
{
    bool hit = false;
    float t, gu, gv;
    float mt = tMax;
    const auto& ts = model->GetTriangles();
    const auto& vs = model->GetVertices();
    for (auto& it : ts) {
        const vec3& a = vs[it.idx[0]].pos;
        const vec3& b = vs[it.idx[1]].pos;
        const vec3& c = vs[it.idx[2]].pos;
        if (Moller_Trumbore(r.o, r.d, a, b, c, t, gu, gv) &&
            t > tMin && t < dis && t < mt && t < tMax) {
            mt = t;
            *isect = Interaction(it, mt);
            isect->tri.bary = { 1 - gu - gv, gu, gv };
            isect->p = r.o + t * r.d;
            hit = true;
            break;
        }
    }
    return hit;
}
