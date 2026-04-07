#pragma once
#include "bvh.h"
#include "model.h"
namespace ry {
class Scene {
public:
    void AddModel(Model m);
    inline void DelModel() {
        
    };
    const std::vector<Model>& GetModels() const { return models; }
    const std::vector<Triangle>& GetTriangles() const { return tris; }

    void SetActiveCamera(uint8_t c) { cameraId = c; };
    const Camera& GetActiveCamera() const { return cameras[cameraId]; };

    void BuildBVH();
private:
    std::vector<Model> models;
    std::vector<Triangle> tris;

    uint8_t cameraId = 0;
    std::vector<Camera> cameras;

    std::vector<BVHNode> bvhNodes;
    uint32_t leafCnt = 0;
};

}