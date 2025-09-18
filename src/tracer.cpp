#include "tracer.h"
#include "algorithm.h"
#include "task.h"
#include "light.h"
#include "log.h"

using namespace std;
using namespace ry;
using namespace glm;
using namespace alg;

// Ambient Shading
vec4 A = vec4(0.051, 0.051, 0.051, 1.0) * 1.0f;

Tracer::Tracer()
{
    maxTraces = 16;
    cout << "use " << maxTraces << " ray for every pixel" << endl;
}

void Tracer::Excute()
{
    for (int i = 1; i <= maxTraces; i++) {
        cout << "start " << currentTraces << "-th rayray" << endl;
        currentTraces = i;
        Parallel();
    }
    sppBuffer.clear();
}

void Tracer::SetInOutPut(const ry::Screen& s, Loader* m, ry::vec4* p)
{
    if (s.h <= 0 && s.w <= 0) {
        // throw ("");
    } else if (scr.h == s.h && scr.w == s.w) {

    } else {
        scr = s;
        tMax = Loader::GetInstance().GetCam().zfar;
        tMin = Loader::GetInstance().GetCam().znear;
        sppBuffer.clear();
        sppBuffer.resize(scr.h * scr.w);
    }
    scr = s;
    pixels = p;
    model = m;
}

void Tracer::Parallel()
{
    auto& t = Task::GetInstance();
#ifdef DEBUG
    auto wCnt = 1;
#else
    auto wCnt = t.WokerCnt();
#endif
    for (auto i = 0; i < wCnt; i ++) {
        t.Add([this, i, &t, h = scr.h / wCnt]() {
            // use the number of pixels on the y-axis to parallel cal
            for (uint16_t y = i * h; y < (i + 1) * h; y++) {
                for (uint16_t x = 0; x < scr.w; x++) {
                    uint32_t num = y * scr.w + x;
#ifdef DEBUG
                    curX = x;
                    curY = y;
#endif
                    sppBuffer[num] += vec4(RayCompute(x, y).c, 1.0);
                    pixels[num] = vec4(vec3(sppBuffer[num]) / (float)currentTraces, 1.0f);
                }
            }
        });
    }
    t.AsynExcute();

    if (auto mod = scr.h % wCnt; mod) {
        for (uint16_t y = scr.h / wCnt * wCnt; y < scr.h; y++) {
            for (uint16_t x = 0; x < scr.w; x++) {
                uint32_t num = y * scr.w + x;
                sppBuffer[num] += vec4(RayCompute(x, y).c, 1.0);
                pixels[num] = vec4(vec3(sppBuffer[num]) / (float)currentTraces, 1.0f);
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

Spectrum Tracer::RayCompute(uint32_t x, uint32_t y)
{
    Ray r = RayGeneration(x, y); 
    Spectrum Lo(0.);
    Lo += Li(r);
    return Lo;
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

Spectrum Tracer::EstimateDirect(const Interaction& isect)
{
    Spectrum Lo(0.);
    auto& lgt = model->GetLgt();
    Sampler s;
    vec3 w;
    return lgt.Sample_Li(s, &isect, w);
}

Spectrum Tracer::Li(const Ray& r)
{
    Spectrum Lo(0.);
    Spectrum beta(1.f);
    Interaction isect;
    Ray ray = r;
    for (int bounce = 0; bounce < 3/*bounces*/; bounce++) {
        Spectrum L(0.);
        // Lo += beta * Le;
        if (!isect.Intersect(ray, model->GetCam().znear, model->GetCam().zfar)) {
            Lo += beta * vec3(A);
            break;
        } else if (model->isEmissive(isect.tri->material)) {
            auto& lgt = model->GetLgt();
            Lo += beta * lgt.I.c * lgt.emissiveStrength;
            break;
        }
#ifdef DEBUG
        /*if (curY >= 300 && curX >= 255 && curX <= 270) {
            Log << "x = " << curX << " y = " << curY << "hit list = ";
            for (auto i : isect.record) {
                Log << i << " ";
            }
            Log << endl;
        }*/
#endif
        if (bounce == 0) {
            // process specular 
        }
        Lo += EstimateDirect(isect);

        const Material& mat = model->GetMaterial(isect.tri->material);
        Spectrum kd = (vec3(mat.pbrMetallicRoughness.baseColorFactor[0],
            mat.pbrMetallicRoughness.baseColorFactor[1], mat.pbrMetallicRoughness.baseColorFactor[2]));

        auto isectVts = model->GetTriVts(*isect.tri);
        vec3 n = normalize(isect.bary[0] * isectVts[0]->normal
            + isect.bary[1] * isectVts[1]->normal + isect.bary[2] * isectVts[2]->normal);
        BSDF b(n);
        b.Add(make_unique<LambertianReflection>(kd));
        // indirect
        Sampler s;
        float pdf;
        vec3 wi;
        Spectrum f = b.Sample_f(-ray.d, &wi, s.Get2D(), &pdf, BSDF_ALL);
        if (pdf <= 0) {
            break;
        }
        // float cos = glm::max(dot(n, wi), 0.f);
        beta *= f * abs(dot(wi, n)) / pdf;
        ray.d = wi;
        ray.o = isect.p + ShadowEpsilon * n;
        /*
            recursive version like:
            Lo += f * Le * cos / pdf;
            Le = Li(new ray);
            Lo += f * Li(new ray) * cos / pdf;
            so there use beta *= f * cos / pdf;
        */
    }
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
