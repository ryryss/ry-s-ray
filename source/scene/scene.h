#pragma once
#include "interaction.hpp"
#include "model.h"
namespace ry {
class Scene {
public:
    void AddModel(std::string file);
    void DelModel();

    void ProcessCamera(uint16_t scrw, uint16_t scrh);
    void SetActiveCamera(uint8_t c) { camera = c; };
    const Camera& GetActiveCamera() { return cameras[camera]; };
    
    const Light& SampleOneLight();

    bool Intersect(const Ray& r, Interaction& isect) const;
    const Light* IntersectEmissive(const Ray& r, Interaction& isect) const;
private:
    void ParseModel(std::shared_ptr<Model> model);

    std::vector<std::shared_ptr<Model>> models;
    std::vector <Light> lights;
    std::vector <Camera> cameras;

    uint8_t camera = 0;
};
}
