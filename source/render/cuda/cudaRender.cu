#include "GpuBackend.h"

using namespace ry;
using namespace std;


__global__ void RenderKernel(const DeviceScene scene, float4* out)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t pixelCount = scene.params.width * scene.params.height;
    if (idx >= pixelCount) {
        return;
    };


}

void Launch(const DeviceScene scene,float4* out)
{
    uint32_t pixelCount = scene.params.width * scene.params.height;
    int blockSize = 256; // 256 = 8 warps
    int gridSize = (pixelCount + blockSize - 1) / blockSize;

    RenderKernel << <gridSize, blockSize >> > (scene, out);

    cudaDeviceSynchronize();
}