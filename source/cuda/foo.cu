
#include <cuda_runtime.h>   // 必须
#include <device_launch_parameters.h> // 推荐（Windows）

#include "gpuscene.h"    // 你自己的 struct（Ray / Triangle）

using namespace ry;
using namespace std;

__host__ __device__
float3 operator-(const float3& a, const float3& b)
{
    return make_float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

__host__ __device__
float3 operator+(const float3& a, const float3& b)
{
    return make_float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

__host__ __device__
float dot(const float3& a, const float3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

__host__ __device__
float3 cross(const float3& a, const float3& b)
{
    return make_float3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

__device__ bool intersectTriangle(
    const GPURay& ray,
    const GPUTriangle& tri,
    float& t)
{
    const float EPS = 1e-6f;

    float3 e1 = tri.v1 - tri.v0;
    float3 e2 = tri.v2 - tri.v0;

    float3 p = cross(ray.d, e2);
    float det = dot(e1, p);

    if (fabs(det) < EPS) return false;

    float inv = 1.0f / det;

    float3 s = ray.o - tri.v0;
    float u = dot(s, p) * inv;
    if (u < 0 || u > 1) return false;

    float3 q = cross(s, e1);
    float v = dot(ray.d, q) * inv;
    if (v < 0 || u + v > 1) return false;

    t = dot(e2, q) * inv;

    return t > EPS;
}

__global__ void renderKernel(
    GPURay* rays,
    GPUTriangle* tris,
    int triCount,
    float3* framebuffer,
    int pixelCount)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= pixelCount) return;

    GPURay ray = rays[idx];

    bool hit = false;
    if (idx == 66)
    {
        printf("ray origin: %f %f %f\n",
            rays[0].o.x, rays[0].o.y, rays[0].o.z);
    }
    for (int i = 0; i < triCount; i++)
    {
        float t;
        if (intersectTriangle(ray, tris[i], t))
        {
            hit = true;
            // printf("idx %d  hit tri %d\n", idx, i);
            break;
        }
    }

    framebuffer[idx] = hit ? make_float3(1, 1, 1)
        : make_float3(0, 0, 0);
}

void launchRender(
    GPURay* d_rays,
    GPUTriangle* d_tris,
    int triCount,
    float3* d_fb,
    int pixelCount)
{
    int blockSize = 256;
    int gridSize = (pixelCount + blockSize - 1) / blockSize;

    renderKernel << <gridSize, blockSize >> > (
        d_rays,
        d_tris,
        triCount,
        d_fb,
        pixelCount
        );

    cudaDeviceSynchronize(); // 调试阶段建议加
}