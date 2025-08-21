#ifndef MODEL_LODER
#define MODEL_LODER
#include <tinygltf/tiny_gltf.h>
#include "pub.h"

class GLBModelLoader {
public:
    bool LoadFromFile(const std::string& filepath);

    std::vector<ry::Triangle>& GetTriangles() {
        return tris;
    }
    ry::Camera& GetCam() {
        return cam;
    }
private:
    void ParsePrimitive(const tinygltf::Primitive& p, const ry::mat4& m);
    void ParseNode();
    void ParseMesh(int num);
    void ParseChildNode(int num);
    void ParseCam(int num);
    void ParseLgt(int num);

    ry::mat4 GetNodeMat(int num);

    tinygltf::Model model;
    std::vector<ry::Triangle> tris;
    std::vector<ry::Node> nodes;
    std::vector<uint32_t> roots;
    ry::Camera cam;
    ry::Light lgt;
};
#endif