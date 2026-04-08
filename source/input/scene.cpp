#include "scene.h"
#include "algorithm.hpp"
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

void Scene::ProcessCamera(uint16_t scrw, uint16_t scrh)
{
    auto& cam = cameras[cameraId];
    if (cam.type == 0) {
        cam.ymag = cam.znear * tan(cam.yfov / 2);
        cam.xmag = cam.ymag * cam.aspectRatio;

        cam.projMatrix = perspective(
            (float)cam.yfov,
            (float)cam.aspectRatio, cam.znear, cam.zfar
        );
        cam.clipToCamera = inverse(cam.projMatrix);
    } else {
        cam.xmag = cam.ymag * scrw / scrh;

        float left = -cam.xmag;
        float right = cam.xmag;
        float bottom = -cam.ymag;
        float top = cam.ymag;
        cam.projMatrix = ortho(
            left, right,
            bottom, top,
            cam.znear, cam.zfar
        );
        cam.clipToCamera = inverse(cam.projMatrix);
    }
    // node.m is camera to world so inverse(node.m) is viewMatrix;
    cam.viewMatrix = inverse(cam.m);
    cam.projView = cam.projMatrix * cam.viewMatrix;
}

void Scene::BuildBVH()
{
    BVHNode root;

}
;