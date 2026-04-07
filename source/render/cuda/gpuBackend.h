#pragma once
#include "IRenderBackend.h"
#ifdef USE_CUDA
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#define CUDA_CHECK(call)                                      \
do {                                                          \
    cudaError_t err = call;                                   \
    if (err != cudaSuccess) {                                 \
        std::cerr << "CUDA Error: "                           \
                  << cudaGetErrorString(err)                  \
                  << " at " << __FILE__ << ":" << __LINE__    \
                  << std::endl;                               \
        exit(1);                                              \
    }                                                         \
} while(0)

namespace ry {
struct SceneParams {
    int triCnt;
    int width;
    int height;
};

struct DeviceScene {
    /*float3* v0 = nullptr;
    float3* v1 = nullptr;
    float3* v2 = nullptr;

    float3* n0 = nullptr;
    float3* n1 = nullptr;
    float3* n2 = nullptr;

    float2* uv0 = nullptr;
    float2* uv1 = nullptr;
    float2* uv2 = nullptr;*/
    Triangle* tris = nullptr;
    int* material = nullptr;

    Camera cam;
    SceneParams params;
};

class GpuBackend : public IRenderBackend {
public:
    void Render(const Scene& scene, RenderTarget& target) override;
private:
    void Upload(const Scene& scene);

    bool dirty = true;
    DeviceScene dScene;
    vec4* out = nullptr;
};
}

#endif