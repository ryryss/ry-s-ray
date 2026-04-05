#pragma once
#include "public.h"

#include <tinygltf/tiny_gltf.h>
#include "model.h"

namespace ry {
namespace gltf = tinygltf;

class IModelLoader {
public:
    virtual ~IModelLoader() = default;
    virtual Model Load(const std::string& filename) = 0;
};

class GLTFLoader : public IModelLoader {
// other formats file may be no need these struct
struct Node {
    std::string name;
    mat4 m = mat4(1.0f); // trans mat
    std::vector<uint32_t> c; // children
    int i = -1;
    std::string type = "node";
};
struct VertexInfo {
    vec3 pos;
    vec3 normal;
    vec2 uv;
    int material;
    vec4 color = vec4(1.0);
};
struct CameraNode : public Node {
    
    gltf::Camera* c = nullptr;
    CameraNode() {};
    CameraNode(const Node& other) : Node(other) {};
};

public:
    Model Load(const std::string& filename) override;
private:
    inline bool IsEmissive(int i) {
        return (i >= 0 && !(raw.materials.size() <= 0) &&
            (raw.materials[i].emissiveFactor[0] > 0.0f ||
             raw.materials[i].emissiveFactor[1] > 0.0f ||
             raw.materials[i].emissiveFactor[2] > 0.0f));
    }

    Model BuildModel();
    Triangle BuildTriangle(const VertexInfo& v0, const VertexInfo& v1, const VertexInfo& v2);

    mat4 GetNodeMat(int num);
    void ParseNode();
    void ParseImage();
    void ParseMesh(int num);
    void ParseChildNode(int num);
    void ParseCamera(int num);
    void ParseLight(int num);
    void ParseMaterial(int num);
    void ParseEmissiveMaterial(int num, const std::vector<uint64_t>& ids);

    std::vector<uint32_t> ParseVertIdx(const gltf::Primitive& p);
    void ParsePrimitive(const gltf::Primitive& p, const mat4& m);
    void ParseTexTureCoord(const gltf::Primitive& p, std::vector<VertexInfo>& vert);
    void ParseNormal(const gltf::Primitive& p, std::vector<VertexInfo>& vert);
    // void ParseVertColor(const gltf::Primitive& p, std::vector<Vertex>& vert);
    void ParsePosition(const gltf::Primitive& p, std::vector<VertexInfo>& vert);

    gltf::Model raw;
    std::vector<Node> nodes;
    std::vector<uint32_t> roots;
    std::vector<VertexInfo> vertices;
    std::vector<uint32_t> vertIdx;
    std::vector<std::vector<uint32_t>> emissiveVertIdx;
    std::vector<CameraNode> cams;
};

class Input {
public:
    static Model Load(const std::string& filename) {
        auto loader = CreateLoader(filename);
        return loader->Load(filename);
    }
private:
    static std::unique_ptr<IModelLoader> CreateLoader(const std::string& filename);
};
}