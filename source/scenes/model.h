#pragma once
#include "geometry.hpp"
#include "light.h"
#include "material.h"
namespace ry {
class Scene;
// tinygltf wrapper
class Model {
public:
    Model(std::string file) { LoadFromFile(file); };
private:
    inline bool IsEmissive(int i) {
        return (i >= 0 && !(model.materials.size() <= 0) &&
               (model.materials[i].emissiveFactor[0] > 0.0f ||
                model.materials[i].emissiveFactor[1] > 0.0f ||
                model.materials[i].emissiveFactor[2] > 0.0f));
    }

	bool LoadFromFile(const std::string& file);
    void ParseNode();
    void ParseMesh(int num);
    void ParseChildNode(int num);
    void ParseCamera(int num);
    void ParseLight(int num);
    void ParseMaterial(int num);
    void ParseEmissiveMaterial(int num, const std::vector<uint64_t>& ids);

    std::vector<uint32_t> ParseVertIdx(const gltf::Primitive& p);
    void ParsePrimitive(const gltf::Primitive& p, const mat4& m);
    void ParseTexTureCoord(const gltf::Primitive& p, std::vector<Vertex>& vert);
    void ParseNormal(const gltf::Primitive& p, std::vector<Vertex>& vert);
    void ParseVertColor(const gltf::Primitive& p, std::vector<Vertex>& vert);
    void ParsePosition(const gltf::Primitive& p, std::vector<Vertex>& vert);

    mat4 GetNodeMat(int num);
    std::vector <Light> lights;
    std::vector <Camera> cameras;
    std::vector <Material> materials;
    // std::vector <Material> mats;
	gltf::Model model;
    std::vector<Node> nodes;
    std::vector<uint32_t> roots;
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;

    friend class Scene;
};
}