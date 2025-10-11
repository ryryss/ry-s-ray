#pragma once
#include "denoiser.h"
#include "scene.h"
namespace ry{
class PathRenderer {
public:
    PathRenderer();
    void Render(Scene* s, uint16_t screenx, uint16_t screeny, vec4* p);
private:
    vec2 ComputeMotionVector(const vec3& worldPos, const mat4& ViewProj_prev, const mat4& ViewProj_curr);
    void Denoising();
    void Parallel();
    Ray RayGeneration(uint32_t x, uint32_t y);
    Spectrum PathTracing(uint32_t x, uint32_t y);
    // or named radiance(), L = Radiance, i = incoming
    Spectrum Li(const Ray& r, PixelInfo* pInf);

    Spectrum EstimateDirect(const vec3& wo, const Interaction& isect);

    vec4* output;
    std::vector<vec3> sppBuffer;
    float tMin, tMax;
    uint16_t maxTraces = 1; // spp
    uint16_t currentTraces = 0;

    mat4 preProjView;
    std::unique_ptr<Denoiser> denoiser;
    std::vector<PixelInfo>* pixelInfos;

    uint16_t scrw, scrh;
    Scene* scene;
};
}