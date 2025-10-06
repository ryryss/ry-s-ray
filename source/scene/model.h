#pragma once
#include "bvh.h"
#include "geometry.hpp"
#include "light.h"
#include "material.h"
namespace ry {
class Scene;
// tinygltf wrapper
class Model {
public:
    Model(std::string file) { LoadFromFile(file); };
    bool Intersect(const Ray& r, const std::vector<uint64_t>& idx, Interaction& isect) const;
    bool Intersect(const Ray& r, Interaction& isect) const;
    inline const Vertex* GetVertex(uint64_t i) const {
        return &vertices[i];
    };

    inline const Triangle* GetTriangle(uint64_t i) const {
        return &triangles[i];
    };

    inline const gltf::Model& GetRaw() const {
        return raw;
    }
private:
    inline bool IsEmissive(int i) {
        return (i >= 0 && !(raw.materials.size() <= 0) &&
               (raw.materials[i].emissiveFactor[0] > 0.0f ||
                raw.materials[i].emissiveFactor[1] > 0.0f ||
                raw.materials[i].emissiveFactor[2] > 0.0f));
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
    gltf::Model raw;
    std::vector<Node> nodes;
    std::vector<uint32_t> roots;
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;

    std::unique_ptr<BVH> bvh;
    friend class BVH;
    friend class Scene;
};
}