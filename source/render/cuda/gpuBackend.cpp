#include "gpuBackend.h"
using namespace std;
using namespace ry;
#ifdef USE_CUDA
extern void Launch(const DeviceScene scene, float4* out);

void GpuBackend::Render(const Scene& scene, RenderTarget& target)
{
    uint32_t pixelCnt = target.width * target.height;
    if (dScene.params.height != target.height || dScene.params.width != target.width) {
        dScene.params.height = target.height;
        dScene.params.width = target.width;
    }
    if (dirty || dScene.params.triCnt != scene.GetTrisCnt()) {
        dScene.params.triCnt = scene.GetTrisCnt();
        Upload(scene);
        dirty = false;
        CUDA_CHECK(cudaMalloc(&out, pixelCnt * sizeof(float4)));
    }
    // upload camera info erveryframe
    dScene.cam = scene.GetActiveCamera();

    Launch(dScene, out);

    // get render result
    for (uint32_t i = 0; i < pixelCnt; i++) {
        target.pixels[i].x = out[i].x;
        target.pixels[i].y = out[i].y;
        target.pixels[i].z = out[i].z;
        target.pixels[i].w = out[i].w;
    }
}

void GpuBackend::Upload(const Scene& scene)
{
    uint32_t n = dScene.params.triCnt;

    cudaMalloc(&dScene.v0, sizeof(float3) * n);
    cudaMalloc(&dScene.v1, sizeof(float3) * n);
    cudaMalloc(&dScene.v2, sizeof(float3) * n);

    cudaMalloc(&dScene.n0, sizeof(float3) * n);
    cudaMalloc(&dScene.n1, sizeof(float3) * n);
    cudaMalloc(&dScene.n2, sizeof(float3) * n);

    cudaMalloc(&dScene.uv0, sizeof(float2) * n);
    cudaMalloc(&dScene.uv1, sizeof(float2) * n);
    cudaMalloc(&dScene.uv2, sizeof(float2) * n);

    cudaMalloc(&dScene.material, sizeof(int) * n);

    // cpu staging buffer
    std::vector<float3> v0(n), v1(n), v2(n);
    std::vector<float3> n0(n), n1(n), n2(n);
    std::vector<float2> uv0(n), uv1(n), uv2(n);
    std::vector<int> mat(n);

    auto models = scene.GetModels();
    uint32_t i = 0;
    for (auto& model : models) {
        for (auto& t : model.GetTriangles()) {
            v0[i] = make_float3(t.v0.x, t.v0.y, t.v0.z);
            v1[i] = make_float3(t.v1.x, t.v1.y, t.v1.z);
            v2[i] = make_float3(t.v2.x, t.v2.y, t.v2.z);

            n0[i] = make_float3(t.n0.x, t.n0.y, t.n0.z);
            n1[i] = make_float3(t.n1.x, t.n1.y, t.n1.z);
            n2[i] = make_float3(t.n2.x, t.n2.y, t.n2.z);

            uv0[i] = make_float2(t.uv0.x, t.uv0.y);
            uv1[i] = make_float2(t.uv1.x, t.uv1.y);
            uv2[i] = make_float2(t.uv2.x, t.uv2.y);

            mat[i] = t.material;

            i < n ? i++ : i = 0;
        }
    }
}

#endif