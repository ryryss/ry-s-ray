#pragma once
#include "IRenderBackend.h"
namespace ry{
class IRenderBackend;

class CpuBackend : public IRenderBackend {
public:
    void Render(Scene& scene, RenderTarget& target) override;
};
}