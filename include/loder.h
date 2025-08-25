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
    inline const std::vector<ry::vec3>& GetPosition() {
        return position;
    }
    inline const std::vector<uint32_t>& GetIndex() {
        return index;
    }
private:
    void ParsePrimitive(const tinygltf::Primitive& p, const ry::mat4& m);
    std::vector<uint32_t> ParseVertIdx(const tinygltf::Primitive& p);
    std::vector<ry::vec2> ParseTexTure(const tinygltf::Primitive& p);
    std::vector<ry::vec3> ParseNormal(const tinygltf::Primitive& p);
    std::vector<ry::vec4> ParseVertColor(const tinygltf::Primitive& p);
    std::vector<ry::vec3> ParsePosition(const tinygltf::Primitive& p);

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

    std::vector<uint32_t> index;
    std::vector<ry::vec2> texture;
    std::vector<ry::vec3> normal;
    std::vector<ry::vec4> vertColor;
    std::vector<ry::vec3> position;
};
#endif