#include "GPUScene.h"
#include "Scene.h"
#include "Model.h"

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

using namespace ry;
using namespace std;

GPUScene::GPUScene(int h, int w) : height(h), width(w) {
    CUDA_CHECK(cudaMalloc(
        &d_fb,
        height * width * sizeof(float3)
    ));
}

void GPUScene::Upload(Scene* scene, const std::vector<ry::Ray>&rays)
{
    std::vector<GPUTriangle> gpuTriangles;

    for (auto& model : scene->models)
    {
        for (auto& tri : model->triangles)
        {
            GPUTriangle t;

            t.v0 = model->vertices[tri.vertIdx[0]].PosToFloat3();
            t.v1 = model->vertices[tri.vertIdx[1]].PosToFloat3();
            t.v2 = model->vertices[tri.vertIdx[2]].PosToFloat3();

            t.n0 = model->vertices[tri.vertIdx[0]].NormalToFloat3();
            t.n1 = model->vertices[tri.vertIdx[1]].NormalToFloat3();
            t.n2 = model->vertices[tri.vertIdx[2]].NormalToFloat3();

            gpuTriangles.push_back(t);
        }
    }

    triangleCount = gpuTriangles.size();

    CUDA_CHECK(cudaMalloc(
        &d_triangles,
        triangleCount * sizeof(GPUTriangle)
    ));

    CUDA_CHECK(cudaMemcpy(
        d_triangles,
        gpuTriangles.data(),
        triangleCount * sizeof(GPUTriangle),
        cudaMemcpyHostToDevice
    ));

// ============================
// lights
// ============================
    const auto l = scene->lights[0];
    GPULight gl;
    GPUTriangle t;
    gpuTriangles.clear();
    for (auto& triIdx : l.triangles) {
        auto tri = scene->models[0]->triangles[triIdx];
        t.v0 = scene->models[0]->vertices[tri.vertIdx[0]].PosToFloat3();
        t.v1 = scene->models[0]->vertices[tri.vertIdx[1]].PosToFloat3();
        t.v2 = scene->models[0]->vertices[tri.vertIdx[2]].PosToFloat3();
        gpuTriangles.push_back(t);
    }
    gl.triCnt = gpuTriangles.size();


    CUDA_CHECK(cudaMalloc(
        &gl.tris,
        gl.triCnt * sizeof(GPUTriangle)
    ));

    CUDA_CHECK(cudaMemcpy(
        gl.tris,
        gpuTriangles.data(),
        gl.triCnt * sizeof(GPUTriangle),
        cudaMemcpyHostToDevice
    ));

    gl.area = l.area;
    gl.emissiveStrength = l.emissiveStrength;

    CUDA_CHECK(cudaMalloc(
        &d_lights,
        sizeof(GPULight)
    ));

    CUDA_CHECK(cudaMemcpy(
        d_lights,
        &gl,
        sizeof(GPULight),
        cudaMemcpyHostToDevice
    ));
    lightCount = 1;

    // ============================
    // rays
    // ============================
    vector<GPURay> gr;
    gr.resize(height * width);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            gr[idx].o = make_float3(rays[idx].o[0], rays[idx].o[1], rays[idx].o[2]);
            gr[idx].d = make_float3(rays[idx].d[0], rays[idx].d[1], rays[idx].d[2]);
        }
    }
    CUDA_CHECK(cudaMalloc(
        &d_rays,
        rays.size() * sizeof(GPURay)
    ));

    CUDA_CHECK(cudaMemcpy(
        d_rays,
        gr.data(),
        rays.size() * sizeof(GPURay),
        cudaMemcpyHostToDevice
    ));
}

extern void launchRender(
    GPURay*,
    GPUTriangle*,
    int,
    float3*,
    int
);

void ry::GPUScene::Render()
{
    launchRender(d_rays, d_triangles, triangleCount, d_fb, height * width);
}
