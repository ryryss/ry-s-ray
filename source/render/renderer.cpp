#include "renderer.h"
#include "cpuBackend.h"
#include "gpuBackend.h"
using namespace std;
using namespace ry;

unique_ptr<IRenderBackend> Renderer::CreateBackend()
{
#ifdef USE_CUDA
    if (useGpu) {
        return std::make_unique<GpuBackend>();
    }
#endif // USE_CUDA
    return std::make_unique<CpuBackend>();
}

void Renderer::Render(const Scene& scene, RenderTarget& target)
{
    backend->Render(scene, target);
}



