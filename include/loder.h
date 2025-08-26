#ifndef LODER
#define LODER
#include <tinygltf/tiny_gltf.h>
#include "pub.h"

class Loader {
public:
    bool LoadFromFile(const std::string& filepath);
    inline ry::Camera& GetCam() {
        return cam;
    }
    inline ry::Light& GetLgt() {
        return lgt;
    }
    inline std::vector <ry::Triangle>& GetTriangles() {
        return triangles;
    }
    inline std::vector <ry::Vertex>& GetVertices() {
        return vertices;
    }
private:
    std::vector<uint32_t> ParseVertIdx(const tinygltf::Primitive& p);
    void ParsePrimitive(const tinygltf::Primitive& p, const ry::mat4& m);
    void ParseTexTure(const tinygltf::Primitive& p, std::vector<ry::Vertex>& vert);
    void ParseNormal(const tinygltf::Primitive& p, std::vector<ry::Vertex>& vert);
    void ParseVertColor(const tinygltf::Primitive& p, std::vector<ry::Vertex>& vert);
    void ParsePosition(const tinygltf::Primitive& p, std::vector<ry::Vertex>& vert);

    void ParseNode();
    void ParseMesh(int num);
    void ParseChildNode(int num);
    void ParseCam(int num);
    void ParseLgt(int num);

    ry::mat4 GetNodeMat(int num);

    tinygltf::Model model;
    std::vector<ry::Node> nodes;
    std::vector<uint32_t> roots;
    ry::Camera cam;
    ry::Light lgt;
    std::vector<ry::Vertex> vertices;
    std::vector<ry::Triangle> triangles;
};
#endif