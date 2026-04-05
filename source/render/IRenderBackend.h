#pragma once
#include "scene.h"
namespace ry {
struct RenderTarget {
    int width, height;
    vec4* pixels = nullptr;
};

class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;
    virtual void Render(const Scene& scene, RenderTarget& target) = 0;
};
}