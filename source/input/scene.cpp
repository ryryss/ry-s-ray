#include "scene.h"
using namespace ry;
using namespace std;

void Scene::AddModel(Model m) {
    trisCnt += m.GetTriangles().size();
    models.push_back(std::move(m));

    const auto& cams = m.GetCameras();
    cameras.insert(cameras.begin(), cams.begin(), cams.end());
};