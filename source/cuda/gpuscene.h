#pragma once
#include "scene.h"
namespace ry {
struct GPUTriangle
{
    float3 v0;
    float3 v1;
    float3 v2;

    float3 n0;
    float3 n1;
    float3 n2;
};

struct GPULight
{
    GPUTriangle* tris;
    int triCnt;
    float area = 0.0f;
    float emissiveStrength;
};

struct GPUCamera
{
    float3 position;
    float3 forward;
    float3 right;
    float3 up;

    float fov;
};

struct GPURay
{
    float3 o;
    float3 d;
};

class GPUScene
{
public:
    GPUScene(int h, int w);
    int height;
    int width;

    GPURay* d_rays = nullptr;
    GPUTriangle* d_triangles = nullptr;
    int triangleCount = 0;

    GPULight* d_lights = nullptr;
    int lightCount = 0;

    float3* d_fb = nullptr;

    GPUCamera camera;

    void Upload(ry::Scene* scene, const std::vector<ry::Ray>& rays);
    void Render();
    void Free();
};
}