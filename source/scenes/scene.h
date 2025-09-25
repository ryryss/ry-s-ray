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

	inline const Vertex* GetVertex(uint64_t i) const {
		return &vertices[i];
	};

	inline const Triangle* GetTriangle(uint64_t i) const {
		return &triangles[i];
	};

	bool Intersect(const Ray& r, Interaction& isect) const;
private:
	void ParseModel(const Model& model);

	std::vector<Model> models;

	std::vector <Light> lights;
	std::vector <Camera> cameras;
	std::vector <Material> materials;
	std::vector<Vertex> vertices;
	std::vector<Triangle> triangles;

	uint16_t x; uint16_t y;
	uint8_t camera = 0;
};
}

