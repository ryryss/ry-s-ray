#pragma once
#include "scene.h"
namespace ry {
struct RenderTarget {
    int16_t width, height;
    vec4* pixels = nullptr;
    RenderTarget(int16_t w, int16_t h) { ReSize(w, h); };
    void ReSize(int16_t w, int16_t h) {
        width = w;
        height = h;
        delete pixels;
        pixels = nullptr;
        pixels = new vec4[w * h];
    };
};

class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;
    virtual void Render(Scene& scene, RenderTarget& target) = 0;
};
}