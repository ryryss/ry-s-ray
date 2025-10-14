#include "path.h"
#include "interaction.hpp"
#include "sampler.hpp"
#include "task.h"
using namespace std;
using namespace ry;
using namespace glm;
// Ambient
vec4 A = vec4(0.051, 0.051, 0.051, 1.0) * 1.0f;

PathRenderer::PathRenderer()
{
    maxTraces = 1;
    cout << "use " << maxTraces << " ray for every pixel" << endl;
    denoisers.push_back(make_unique<TemporalDenoiser>(ivec2(3)));
    denoisers.push_back(make_unique<AtrousDenoiser>(ivec2(3)));
}

void PathRenderer::Render(Scene* s, uint16_t screenx, uint16_t screeny, vec4* p)
{
    cam = &s->GetActiveCamera();

    if (screenx <= 0 && screeny <= 0) {
        // throw ("");
    } else if (scrw == screenx && scrh == screeny) {

    } else {
        s->ProcessCamera(screenx, screeny);
        scrw = screenx;
        scrh = screeny;
        tMax = cam->zfar;
        tMin = cam->znear;
        sppBuffer.clear();
        sppBuffer.resize(scrw * scrh);

        gBuffer.resize(scrw * scrh);
        for (auto& d : denoisers) {
            d->ReSize(scrw, scrh);
        }
    }
    prevProjView = cam->projView;
    output = p;
    scene = s;
    for (int i = 1; i <= maxTraces; i++) {
        cout << "start " << currentTraces << "-th rayray" << endl;
        currentTraces = i;
        PathTracing();
        Denoising();
    }
    sppBuffer.clear();
    sppBuffer.resize(scrw * scrh);
}

void ry::PathRenderer::Denoising()
{ 
    /*TemporalDenoiser* temporalDenoiser = (TemporalDenoiser*)denoisers[0].get();
    ParallelDynamic(32, [&](uint16_t x, uint16_t y) {
        auto idx = y * scrw + x;
        const auto& temporalRes = temporalDenoiser->Denoise(x, y, gBuffer);
        output[idx] = vec4(temporalRes, 1.0);
        output[idx] = pow(vec4(temporalRes, 1.0), vec4(GammaSRGB));
    });
    temporalDenoiser->AfterDenoise();
    temporalDenoiser->KeepGBuffer(gBuffer);*/

    AtrousDenoiser* atrousDenoiser = (AtrousDenoiser*)denoisers[1].get();
    auto& ping = atrousDenoiser->GetPing();
    for (int y = 0; y < scrh; y++) {
        for (int x = 0; x < scrw; x++) {
            int idx = y * scrw + x;
            ping[idx] = output[idx];
            // output[idx] *= 0.6; // temporal denoise weight is 0.6
        }
    }
    constexpr int iteration = 3;
    atrousDenoiser->SetTteration(iteration);
    for (int iter = 0; iter < iteration; ++iter) {
        ParallelDynamic(32, [&](uint16_t x, uint16_t y) {
            auto idx = y * scrw + x;
            atrousDenoiser->Denoise(x, y, gBuffer);
        });
        atrousDenoiser->SwapPingPong();
    }

    const auto& res = atrousDenoiser->GetPong();
    for (int y = 0; y < scrh; y++) {
        for (int x = 0; x < scrw; x++) {
            int idx = y * scrw + x;
            // temporal denoise weight is 0.6
            output[idx] = pow(vec4(res[idx], 1.0), vec4(GammaSRGB));
        }
    }
}

void ry::PathRenderer::ParallelDynamic(uint16_t blockSize, function<void(uint16_t x, uint16_t y)> work)
{
    auto& t = Task::GetInstance();
#ifdef DEBUG
    auto wCnt = 10;
#else
    auto wCnt = t.WokerCnt();
#endif
    uint32_t totalBlocks = (scrh + blockSize - 1) / blockSize;
    std::atomic<uint32_t> nextBlock{ 0 };

    for (uint32_t i = 0; i < wCnt; i++) {
        t.Add([&, i]() {
            while (true) {
                uint32_t blockIdx = nextBlock.fetch_add(1);
                if (blockIdx >= totalBlocks) {
                    break;
                }

                uint32_t yStart = blockIdx * blockSize;
                uint32_t yEnd = std::min(yStart + blockSize, (uint32_t)scrh);
                for (uint16_t y = yStart; y < yEnd; y++) {
                    for (uint16_t x = 0; x < scrw; x++) {
                        work(x, y);
                    }
                }
            }
        });
    }
    t.Excute();
}

Ray PathRenderer::RayGeneration(uint32_t x, uint32_t y)
{
    vec3 o, d;
    // Geometric Method
    // l = -xmag  r = xmag b = -ymag t = ymag
    // u = l + (r − l)(i + 0.5)/nx
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
    // display -> NDC -> clip(projection) -> camera(view) -> world
    Sampler s;
    vec2 ndc = (vec2(x, y) + s.Get2D());
    ndc = 2.f * ndc / vec2(scrw, scrh) - 1.f;
    vec4 clip { ndc, -1, 1 };
    vec4 camSpace = cam->clipToCamera * clip;
    camSpace /= camSpace.w;
    // now in camera coordinates
    vec3 dir = cam->m * camSpace /* - vec3(0, 0, 0) */;
    o = cam->e;
    d = normalize(dir - o); // here the dir represents a point
    return { o, d };
}

void PathRenderer::UpdateGBuffer(const Interaction& isect, PixelInfo* pInf)
{
    auto WorldToNdc = [this](const vec3& worldPos, const mat4& viewProj) {
        vec4 clip = viewProj * vec4(worldPos, 1.0);
        return clip /= clip.w;        
    };
    auto NdcToScreen = [&](const vec3& ndcPos) {
        return vec2( (ndcPos.x * 0.5f + 0.5f) * scrw,
                     (1.0f - (ndcPos.y * 0.5f + 0.5f)) * scrh );
    };

    vec4 currClip = WorldToNdc(isect.p, cam->projView);
    vec4 prevClip = WorldToNdc(isect.p, prevProjView);
    pInf->depth = (currClip.z * 0.5f + 0.5f);

    vec2 currScreen = NdcToScreen(currClip);
    vec2 prevScreen = NdcToScreen(prevClip);
    pInf->motion = prevScreen - currScreen;

    pInf->normal = isect.normal;
    pInf->albedo = isect.mat->GetAlbedo();
}

void PathRenderer::PathTracing()
{
    ParallelDynamic(16, [this](uint16_t x, uint16_t y) {
        auto idx = y * scrw + x;
        auto& color = Li(RayGeneration(x, y), &gBuffer[idx]);
        sppBuffer[idx] += color.c;
        gBuffer[idx].color = sppBuffer[idx] / (float)currentTraces;
        output[idx] = vec4(gBuffer[idx].color, 1.0f);
    });
}

Spectrum PathRenderer::Li(const Ray& r, PixelInfo* pInf)
{
    auto& cam = scene->GetActiveCamera();
    Spectrum Lo(0.);
    Spectrum beta(1.f);
    Interaction isect;
    Ray ray = r;
    bool specularBounce = false;
    for (int bounce = 0; bounce < 3/*bounces*/; bounce++) {
        isect.tMin = tMin;
        isect.tMax = tMax;
        // Lo += beta * Le;
        if (!scene->Intersect(ray, isect)) {
            Lo += beta * vec3(A);
            break;
        } 
        
        if (bounce == 0) {
            UpdateGBuffer(isect, pInf);
        }
        
        if ((bounce == 0 || specularBounce) && isect.mat->IsEmissive()) {
            Lo += beta * vec3(isect.mat->GetEmission()); // TODO need to cal angle
        }
        
        if (float back = dot(-ray.d, isect.normal); back <= 0) {
            Lo += beta * vec3(isect.mat->GetAlbedo()) * abs(back);
            break;
        }

        if (isect.bsdf->NumComponents(BxDFType(BSDF_ALL & ~BSDF_SPECULAR)) > 0) {
            Lo += beta * EstimateDirect(ray.d, isect);
        }
        // indirect
        Sampler s;
        float pdf;
        vec3 wi;
        // get wi and f from bsdf
        // r.d from camara and point to hit point
        // wi will start from the hit point
        Spectrum f = isect.bsdf->Sample_f(-ray.d, &wi, s.Get2D(), &pdf);
        if (pdf <= 0) {
            break;
        }
        // float cos = glm::max(dot(n, wi), 0.f);
        vec3 n = isect.normal;
        beta *= f * abs(dot(wi, n)) / pdf;
        ray = Ray{ isect.p , wi };
        specularBounce =  isect.mat->IsSpecular();
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
    Spectrum Ld(0.f);
    auto& light = scene->SampleOneLight();
    Sampler s;
    vec3 wi;
    float pdf;
    Spectrum Li = light.Sample_Li(&isect, &wi, &pdf);
    float cosp = glm::max(0.f, dot(isect.normal, wi));
    Spectrum f = isect.bsdf->f(-wo, wi);
    Ld += Li * f * cosp / pdf;
    // TODO: MIS
    return Ld;
}