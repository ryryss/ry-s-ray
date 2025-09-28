#pragma once
#include "scene.h"
namespace ry{
class PathRenderer {
public:
    PathRenderer();
    void Render(Scene* s, uint16_t screenx, uint16_t screeny, vec4* p);
private:
    void Parallel();
    Ray RayGeneration(uint32_t x, uint32_t y);
    Spectrum PathTracing(uint32_t x, uint32_t y);
    // or named radiance(), L = Radiance, i = incoming
    Spectrum Li(const Ray& r);
    vec4 SampleTexture(const vec3& bary, const vec2& uv0, 
        const vec2& uv1, const vec2& uv2);

    Spectrum EstimateDirect(const Interaction& isect);

    vec4* pixels;
    std::vector<vec4> sppBuffer;
    float tMin, tMax;
    uint16_t maxTraces = 16; // spp
    uint16_t currentTraces = 0;

#ifdef DEBUG
    uint16_t curX;
    uint16_t curY;
#endif
    uint16_t scrw, scrh;
    Scene* scene;
};
}