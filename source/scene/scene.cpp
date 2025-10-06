#include "scene.h"
#include "interaction.hpp"
#include "sample.hpp"
using namespace ry;
using namespace std;

void Scene::AddModel(string file)
{
    shared_ptr<Model> model = make_unique<Model>(file);
    ParseModel(model);
    models.push_back(model);
}

void Scene::DelModel()
{
}

void Scene::ProcessCamera(uint16_t scrw, uint16_t scrh)
{
    for (auto& cam : cameras) {
        if (cam.type == "perspective") {
            cam.ymag = cam.znear * tan(cam.yfov / 2);
            cam.xmag = cam.ymag * cam.aspectRatio;
        } else {
            cam.xmag = cam.ymag * scrw / scrh;
        }
        cam.clipToCamera = glm::inverse(glm::perspective(
            (float)cam.yfov,
            (float)cam.aspectRatio, cam.znear, cam.zfar
        ));
    }
}

const Light& Scene::SampleOneLight()
{
    Sampler s;
    return lights[s.GetIntInRange(0, lights.size() - 1)];
}

bool Scene::Intersect(const Ray& r, Interaction& isect) const
{
    bool hit = false;
    for (auto& model : models) {
        hit |= model->Intersect(r, isect);
    }
    return hit;
}

const Light* ry::Scene::IntersectEmissive(const Ray& r, Interaction& isect) const
{
    const Light* light = nullptr;
    for (int i = 0; i < lights.size(); i++) {
        auto& l = lights[i];
        if (l.model->Intersect(r, l.triangles, isect)) {
            light = &l;
        }
    }
    return light;
}

void Scene::ParseModel(std::shared_ptr<Model> model)
{ 
    cameras.insert(cameras.begin(), model->cameras.begin(), model->cameras.end());
    lights.insert(lights.begin(), model->lights.begin(), model->lights.end());
}