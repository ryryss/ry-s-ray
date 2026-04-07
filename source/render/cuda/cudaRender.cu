#include "GpuBackend.h"
#include "algorithm.hpp"
using namespace ry;
using namespace std;

__global__ void RenderKernel(const DeviceScene scene, vec4* out)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t width = scene.params.width;
    uint32_t height = scene.params.height;
    uint32_t x = idx % width;
    uint32_t y = idx / width;
    uint32_t pixelCount = width * height;

    if (idx >= pixelCount) {
        return;
    };

    Ray r = RayGeneration(x, y, width, height, scene.cam);
    Interaction isect;
    if (Intersect(scene.tris, scene.params.triCnt, r, isect)) {
        out[idx] = vec4(0.6, 0.5, 0.3, 0.3);
    }
}

void Launch(const DeviceScene& scene, vec4* out)
{
    uint32_t pixelCount = scene.params.width * scene.params.height;
    int blockSize = 256; // 256 = 8 warps
    int gridSize = (pixelCount + blockSize - 1) / blockSize;
    cout << scene.params.triCnt << endl;
    RenderKernel << <gridSize, blockSize >> > (scene, out);
    cudaError_t err = cudaDeviceSynchronize();
    cudaDeviceSynchronize();
}