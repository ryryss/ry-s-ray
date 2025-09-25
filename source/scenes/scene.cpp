#include "scene.h"
#include "interaction.hpp"
using namespace ry;
using namespace std;

void Scene::AddModel(string file)
{
	models.push_back(Model(file));
	ParseModel(models.back());
}

void ry::Scene::DelModel()
{
}

bool ry::Scene::Intersect(const Ray& r, Interaction& isect)
{
	bool hit = false;
	float t, gu, gv;
	isect.tMax;
	for (auto& tri : triangles) {
		auto& a = vertices[tri.vertIdx[0]].pos;
		auto& b = vertices[tri.vertIdx[1]].pos;
		auto& c = vertices[tri.vertIdx[2]].pos;
		if (Moller_Trumbore(r.o, r.d, a, b, c, t, gu, gv) &&
			t > isect.tMin && t < isect.tMax) {
			isect.tMax = t;
			isect.bary = { 1 - gu - gv, gu, gv };
			isect.p = r.o + t * r.d;
			isect.tri = &tri;
			hit = true;
		}
#ifdef DEBUG
		else {
			isect.record.push_back(tri.i);
		}
#endif
	}
	if (hit) {
		auto& a = vertices[isect.tri->vertIdx[0]];
		auto& b = vertices[isect.tri->vertIdx[1]];
		auto& c = vertices[isect.tri->vertIdx[2]];
		isect.normal = normalize(isect.bary[0] * a.normal
			+ isect.bary[1] * b.normal + isect.bary[2] * c.normal);
		
		isect.mat = &materials[isect.tri->material];
		// isect.mat->CreateBSDF();
	}
	return hit;
}

void ry::Scene::ParseModel(const Model& model)
{
	vertices.insert(vertices.begin(), model.vertices.begin(), model.vertices.end());
	triangles.insert(triangles.begin(), model.triangles.begin(), model.triangles.end());
	cameras.insert(cameras.begin(), model.cameras.begin(), model.cameras.end());
	lights.insert(lights.begin(), model.lights.begin(), model.lights.end());
	materials.insert(materials.begin(), model.materials.begin(), model.materials.end());
}
