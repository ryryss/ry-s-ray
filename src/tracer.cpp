#include "tracer.h"
#include "algorithm.h"
#include "task.h"

using namespace std;
using namespace ry;
using namespace glm;
using namespace alg;
uint8_t Tracer::maxDeep = 5;

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
    auto w_cnt = t.WokerCnt();
    for (auto i = 0; i < w_cnt; i ++) {
        t.Add([this, i, &t, h = scr.h / w_cnt]() {
            // use the number of pixels on the y-axis to parallel cal
            for (uint16_t y = i * h; y < (i + 1) * h; y++) {
                for (uint16_t x = 0; x < scr.w; x++) {
                    auto r = RayGeneration(x, y);
                    auto rad = Li(r, 0);
                    pixels[y * scr.w + x] = vec4(rad, 1);
                }
            }
        });
    }
    t.AsynExcute();

    if (auto mod = scr.h % w_cnt; mod) {
        for (uint16_t y = scr.h / w_cnt * w_cnt; y < scr.h; y++) {
            for (uint16_t x = 0; x < scr.w; x++) {
                auto r = RayGeneration(x, y);
                auto rad = Li(r, 0);
                pixels[y * scr.w + x] = vec4(rad, 1);
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
    Ray r;
    auto& cam = model->GetCam();
    float uu = -cam.xmag + 2 * cam.xmag * (x + 0.5) / scr.w;
    float vv = -cam.ymag + 2 * cam.ymag * (y + 0.5) / scr.h;
    if (cam.type == "perspective") {
        r.o = cam.e;
        r.d = normalize(cam.znear * cam.w + cam.u * uu + cam.v * vv);
    } else {
        r.o = cam.e + cam.u * uu + cam.v * vv;
        r.d = cam.w;
    }
    return r;
}

// 输入: 随机变量 u ∈ [0,1]^2
vec3 CosineSampleHemisphere(const vec2& u) {
    float r = sqrt(u.x);
    float theta = 2.0f * M_PI * u.y;

    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(std::max(0.0f, 1 - x * x - y * y));

    return vec3(x, y, z);  // 注意：局部坐标系下的向量
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

// Ambient Shading
vec4 A = vec4(0.051, 0.051, 0.051, 1.0) * 1.0f; // ka * Ia, A = env light
Spectrum Tracer::Li(const Ray& r, int depth)
{
    Spectrum res(0.);
    if (++depth > maxDeep) {
        return res;
    }
    Interaction* isect;
    float hit = Intersect(r, tMax, isect);

    // process no hit
    if (!hit && depth == 1) {
        return A;
    } else if (!hit) {
        return res;
    } 
    if (depth == 1) {
        // process self-luminous objects
    }
    res.c += EstimateDirect(*isect);
    // Sample BSDF to get new path direction
    // vec3 c = bxdf->f(r.d, &wi, n, nullptr);
    const vec3& wo = r.d;

    vec3 localWi = CosineSampleHemisphere(u); // 局部坐标
    if (localWi.z < 0) localWi.z *= -1; // 保证在上半球
    wi = ToWorld(localWi, n);
    // path trace
    res += Li(Ray{ hitPoint, wi }, depth); // now ingonor energe lose
    return res; 
}

Spectrum Tracer::EstimateDirect(const Interaction& isect)
{
    Spectrum Ld(0.);
    Sampler s;
    vec2 u = s.Get2D();
    auto& lgt = model->GetLgt();
    vec3 areap = lgt.Sample_Li(isect, u, 1.0);
    vec3 wi = areap - isect.p;
    float dist2 = length(wi);
    wi = normalize(wi);
    Ld += lgt.I.c / dist2;

    const auto& ts = model->GetTriangles();
    const auto& vs = model->GetVertices();
    const auto& ta = vs[isect.t.idx[0]];
    const auto& tb = vs[isect.t.idx[1]];
    const auto& tc = vs[isect.t.idx[2]];
    vec3 n = normalize(isect.t.bary[0] * ta.normal + isect.t.bary[1] * tb.normal + isect.t.bary[2] * tc.normal);
    Interaction isect2;
    if (Intersect(Ray{ isect.p + n * 1e-4f, wi }, dist2, &isect2)) {
        return Spectrum (0.); // shadow
    }
    Ld.c += LambertianShading(SampleTexture(isect.t.bary, ta.uv, tb.uv, tc.uv), 1.0/*lgt.intensity * distance*/, wi, n);
    return Ld;
}

bool Tracer::Intersect(const Ray& r, const float dis, Interaction* isect)
{
    float t, gu, gv;
    float mt = tMax;
    const auto& ts = model->GetTriangles();
    const auto& vs = model->GetVertices();
    for (auto& it : ts) {
        const vec3& a = vs[it.idx[0]].pos;
        const vec3& b = vs[it.idx[1]].pos;
        const vec3& c = vs[it.idx[2]].pos;
        if (Moller_Trumbore(r.o, r.d, a, b, c, t, gu, gv) && 
            t > tMin && t < dis && t < mt && t < tMax ) {
            mt = t;
            *isect = Interaction(it, mt);
            isect->t.bary = { 1 - gu - gv, gu, gv };
            isect->p = r.o + t * r.d;
            return true;
        }
    }
    return false;
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
