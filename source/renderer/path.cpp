#include "path.h"
#include "interaction.hpp"
#include "sample.hpp"
#include "task.h"
using namespace std;
using namespace ry;
using namespace glm;
// Ambient
vec4 A = vec4(0.051, 0.051, 0.051, 1.0) * 1.0f;

PathRenderer::PathRenderer()
{
    maxTraces = 12;
    cout << "use " << maxTraces << " ray for every pixel" << endl;
}

void PathRenderer::Render(Scene* s, uint16_t screenx, uint16_t screeny, vec4* p)
{
    auto& cam = s->GetActiveCamera();
    if (screenx <= 0 && screeny <= 0) {
        // throw ("");
    } else if (scrw == screenx && scrh == screeny) {

    } else {
        s->ProcessCamera(screenx, screeny);
        scrw = screenx;
        scrh = screeny;
        tMax = cam.zfar;
        tMin = cam.znear;
        sppBuffer.clear();
        sppBuffer.resize(scrw * scrh);
    }
    pixels = p;
    scene = s;
    for (int i = 1; i <= maxTraces; i++) {
        cout << "start " << currentTraces << "-th rayray" << endl;
        currentTraces = i;
        Parallel();
    }
    sppBuffer.clear();
}

void PathRenderer::Parallel()
{
    auto& t = Task::GetInstance();
#ifdef DEBUG
    auto wCnt = 1;
#else
    auto wCnt = t.WokerCnt();
#endif
    for (auto i = 0; i < wCnt; i++) {
        t.Add([this, i, h = scrh / wCnt]() {
            // use the number of pixels on the y-axis to parallel cal
            for (uint16_t y = i * h; y < (i + 1) * h; y++) {
                for (uint16_t x = 0; x < scrw; x++) {
                    uint32_t num = y * scrw + x;
#ifdef DEBUG
                    curX = x;
                    curY = y;
                    // if (x >= 200 && x <= 300 && y >= 200 && y <= 400) {
                    vec3 color = PathTracing(x, y).c;
                    sppBuffer[num] += vec4(pow(color, vec3(GammaInv)), 1.0);
                        pixels[num] = vec4(vec3(sppBuffer[num]) / (float)currentTraces, 1.0f);
                    // }
#else
                    vec3 color = PathTracing(x, y).c;
                    sppBuffer[num] += vec4(pow(color, vec3(GammaInv)), 1.0);
                    pixels[num] = vec4(vec3(sppBuffer[num]) / (float)currentTraces, 1.0f);
#endif
                }
            }
        });
    }
    t.AsynExcute();

    if (auto mod = scrh % wCnt; mod) {
        for (uint16_t y = scrh / wCnt * wCnt; y < scrh; y++) {
            for (uint16_t x = 0; x < scrw; x++) {
                uint32_t num = y * scrw + x;
                vec3 color = PathTracing(x, y).c;
                sppBuffer[num] += vec4(pow(color, vec3(GammaInv)), 1.0);
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

Ray PathRenderer::RayGeneration(uint32_t x, uint32_t y)
{
    auto& cam = scene->GetActiveCamera();
    vec3 o, d;
    // Geometric Method
    /*float uu = -cam.xmag + 2 * cam.xmag * (x + 0.5) / scrw;
    float vv = -cam.ymag + 2 * cam.ymag * (y + 0.5) / scrh;
    if (cam.type == "perspective") {
        o = cam.e;
        d = normalize(cam.znear * cam.w + cam.u * uu + cam.v * vv);
    } else {
        o = cam.e + cam.u * uu + cam.v * vv;
        d = cam.w;
    }*/

    // Inverse Projection Method
    // display -> NDC -> clip -> camera -> world
    Sampler s;
    vec2 ndc = (vec2(x, y) + s.Get2D());
    ndc = 2.f * ndc / vec2(scrw, scrh) - 1.f;
    vec4 clip { ndc, -1, 1 };
    vec4 camSpace = cam.clipToCamera * clip;
    camSpace /= camSpace.w;
    // now in camera coordinates
    vec3 dir = cam.m * camSpace /* - vec3(0, 0, 0) */;
    o = cam.e;
    d = normalize(dir - o); // here the dir represents a point
    return { o, d };
}

Spectrum PathRenderer::PathTracing(uint32_t x, uint32_t y)
{
    Spectrum Lo(0.);
    Lo += Li(RayGeneration(x, y));
    return Lo;
}

Spectrum PathRenderer::Li(const Ray& r)
{
    Spectrum Lo(0.);
    Spectrum beta(1.f);
    Interaction isect;
    Ray ray = r;
    for (int bounce = 0; bounce < 3/*bounces*/; bounce++) {
        isect.tMin = tMin;
        isect.tMax = tMax;
        // Lo += beta * Le;
        if (!scene->Intersect(ray, isect)) {
            Lo += beta * vec3(A);
            break;
        } else if (bounce == 0 && isect.mat->IsEmissive()) {
            Lo += beta * vec3(isect.mat->GetEmission());
            // break;
        } else if (float back = dot(-ray.d, isect.tri->normal); back <= 0) {
            Lo += beta * vec3(isect.mat->GetAlbedo()) * abs(back);
            break;
        }

        if (bounce == 0) {
            // process specular 
        }
        Lo += beta * EstimateDirect(ray.d, isect);

        // indirect
        Sampler s;
        float pdf;
        vec3 wi;
        // get wi and f from bsdf
        Spectrum f = isect.bsdf->Sample_f(-ray.d, &wi, s.Get2D(), &pdf, BSDF_ALL);
        if (pdf <= 0) {
            break;
        }
        // float cos = glm::max(dot(n, wi), 0.f);
        vec3 n = isect.normal;
        beta *= f * abs(dot(wi, n)) / pdf;
        ray = Ray{ isect.p , wi };
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

Spectrum PathRenderer::EstimateDirect(const vec3& wo, const Interaction& isect)
{
    // TODO: MIS
    Spectrum Ld(0.f);
    auto& light = scene->SampleOneLight();
    Sampler s;
    vec3 wi;
    float pdf;
    Spectrum Li = light.Sample_Li(scene, &isect, &wi, &pdf);
    float cosp = glm::max(0.f, dot(isect.normal, wi));
    Spectrum f = isect.bsdf->f(wo, wi);
    return Li * f * cosp / pdf;
}