#pragma once
#include "denoiser.h"
#include "renderer.hpp"
namespace ry{
class PathRenderer : public Renderer {
public:
    PathRenderer();
    void Render(Scene* s) override;
private:
    void UpdateSize();
    void UpdateGBuffer(const Interaction& isect, PixelInfo* pInf);
    void Denoising();
    Ray RayGeneration(uint32_t x, uint32_t y);
    void PathTracing();
    // or named radiance(), L = Radiance, i = incoming
    Spectrum Li(const Ray& r, PixelInfo* pInf);

    Spectrum EstimateDirect(const vec3& wo, const Interaction& isect);

    vec4* output;
    std::vector<Spectrum> sppBuffer;
    float tMin, tMax;
    uint16_t maxTraces = 1; // spp
    uint16_t currentTraces = 0;

    mat4 prevProjView;
    std::vector<std::unique_ptr<Denoiser>> denoisers;
    std::vector<PixelInfo> gBuffer;

    const Camera* cam;
};
}