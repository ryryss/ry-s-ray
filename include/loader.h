#ifndef LOADER
#define LOADER
#include "pub.h"
#include "light.h"
#include "algorithm.h"
#include "BVH.h"
class Loader {
public:
    Loader(const Loader&) = delete;
    Loader& operator=(const Loader&) = delete;
    static Loader& GetInstance() {
        static Loader instance;
        return instance;
    }
    void ProcessCamera(const ry::Screen& scr);

    bool LoadFromFile(const std::string& filepath);
    inline ry::Camera& GetCam() {
        return cam;
    }
    inline ry::Light& GetLgt() {
        return lgts.back();
    }
    inline std::vector <ry::Triangle>& GetTriangles() {
        return triangles;
    }
    inline std::vector <ry::Vertex>& GetVertices() {
        return vertices;
    }
    inline std::array<const ry::Vertex*, 3> GetTriVts(const ry::Triangle& t) const {
        return { &vertices[t.vertIdx[0]], &vertices[t.vertIdx[1]], &vertices[t.vertIdx[2]] };
    }
    inline std::array<const ry::Vertex*, 3> GetTriVts(int triIdx) const {
        return GetTriVts(triangles[triIdx]);
    }
    inline tinygltf::Image GetTexTureImg() {
        return model.images.size() <= 0 ? tinygltf::Image() : model.images[0]; // TODO
    }
    inline const ry::Material GetMaterial(int i) {
        return model.materials.size() <= 0 ? ry::Material() : model.materials[i];
    }
    inline const std::shared_ptr<BVH> GetBvh() {
        return bvh;
    }

    inline bool isEmissive(int i) {
        return (!(model.materials.size() <= 0) &&
                (model.materials[i].emissiveFactor[0] > 0.0f ||
                 model.materials[i].emissiveFactor[1] > 0.0f ||
                 model.materials[i].emissiveFactor[2] > 0.0f));
    }

private:
    Loader() {}

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
    std::vector <ry::Light> lgts;
    std::vector<ry::Vertex> vertices;
    std::vector<ry::Triangle> triangles;

    float tMin, tMax;
    std::shared_ptr<BVH> bvh;
};
#endif