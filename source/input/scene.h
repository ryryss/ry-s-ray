#pragma once
#include "model.h"
namespace ry {
class Scene {
public:
    inline void AddModel(Model m) {
        trisCnt += m.GetTriangles().size();
        models.push_back(std::move(m));
    };
    inline void DelModel() {
        
    };
    const std::vector<Model>& GetModels() const { return models; }
    uint32_t GetTrisCnt() const { return trisCnt; }
private:
    uint32_t trisCnt = 0;
    std::vector<Model> models;
};

}