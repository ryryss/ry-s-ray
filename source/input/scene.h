#pragma once
#include "model.h"
namespace ry {
class Scene {
public:
    void AddModel(Model m);
    inline void DelModel() {
        
    };
    const std::vector<Model>& GetModels() const { return models; }
    uint32_t GetTrisCnt() const { return trisCnt; }

    void SetActiveCamera(uint8_t c) { cameraId = c; };
    const Camera& GetActiveCamera() const { return cameras[cameraId]; };
private:
    uint32_t trisCnt = 0;
    std::vector<Model> models;

    uint8_t cameraId = 0;
    std::vector<Camera> cameras;
};

}