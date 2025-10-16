#pragma once
#include "denoiser.h"
#include "scene.h"
namespace ry{
class PathRenderer {
public:
    PathRenderer();
    void Render(Scene* s, uint16_t screenx, uint16_t screeny, vec4* p);
private:
    void UpdateGBuffer(const Interaction& isect, PixelInfo* pInf);
    void Denoising();
    Ray RayGeneration(uint32_t x, uint32_t y);
    void PathTracing();
    // or named radiance(), L = Radiance, i = incoming
    Spectrum Li(const Ray& r, PixelInfo* pInf);

    Spectrum EstimateDirect(const vec3& wo, const Interaction& isect);

    vec4* output;
    std::vector<vec3> sppBuffer;
    float tMin, tMax;
    uint16_t maxTraces = 1; // spp
    uint16_t currentTraces = 0;

    mat4 prevProjView;
    std::vector<std::unique_ptr<Denoiser>> denoisers;
    std::vector<PixelInfo> gBuffer;

    const Camera* cam;

    uint16_t scrw, scrh;
    Scene* scene;
};
}