#pragma once
#include "interaction.hpp"
#include "model.h"
namespace ry {
class Scene {
public:
	void AddModel(std::string file);
	void DelModel();
	void SetActiveCamera(uint8_t c) { camera = c; };
	const Camera& GetActiveCamera() { return cameras[camera]; };
	bool Intersect(const Ray& r, Interaction& isect);
private:
	void ParseModel(const Model& model);

	std::vector<Model> models;

	std::vector <ry::Light> lights;
	std::vector <Camera> cameras;
	std::vector <ry::Material> materials;
	std::vector<Vertex> vertices;
	std::vector<Triangle> triangles;

	uint16_t x; uint16_t y;
	uint8_t camera;
};
}

