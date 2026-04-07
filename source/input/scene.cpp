#include "scene.h"
using namespace ry;
using namespace std;

void Scene::AddModel(Model m) {
    auto mTris = m.GetTriangles();
    // TODO: map tri idx to model for remove model
    tris.insert(tris.begin(), mTris.begin(), mTris.end());

    // TODO: active camera
    const auto& cams = m.GetCameras();
    cameras.insert(cameras.begin(), cams.begin(), cams.end());

    models.push_back(std::move(m));
}

void Scene::BuildBVH()
{
    BVHNode root;

}
;