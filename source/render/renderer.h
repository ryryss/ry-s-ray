#pragma once
#include "IRenderBackend.h"
namespace ry {
class Renderer {
public:
    Renderer() { backend = CreateBackend(); }
    void Render(Scene& scene, RenderTarget& target);

private:
    std::unique_ptr<IRenderBackend> CreateBackend();

    bool useGpu = true;
    std::unique_ptr<IRenderBackend> backend;
};
}