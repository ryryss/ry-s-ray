#ifndef PUB
#define PUB
#define GLM_ENABLE_EXPERIMENTAL
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <array>
#include <chrono>
#include <random>
static float ShadowEpsilon = 0.0001f;
static float Pi = 3.14159265358979323846;
static float InvPi = 0.31830988618379067154;
static float Inv2Pi = 0.15915494309189533577;
static float Inv4Pi = 0.07957747154594766788;
static float PiOver2 = 1.57079632679489661923;
static float PiOver4 = 0.78539816339744830961;
static float Sqrt2 = 1.41421356237309504880;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/random.hpp>

#include <tinygltf/tiny_gltf.h>

namespace ry {
using vec3 = glm::vec3; // can ez change mat lib
using vec2 = glm::vec2;
using vec4 = glm::vec4;
using mat4 = glm::mat4;

class Sampler {
public:
    Sampler() {}
    ry::vec2 Get2D() const {
        return ry::vec2(glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.0f));
    }
    float Get1D() const {
        return glm::linearRand(0.0f, 1.0f);
    }
};

struct Vertex {
    vec3 pos;
    vec4 color = vec4(1.0);
    vec3 normal;
    vec2 uv;
    int i;
};

struct Triangle {
    vec4 color;
    int material;
    Vertex* vts[3];
    vec3 normal;
    int i;
};

struct Node {
    Node* p = nullptr;        // parent, no where use this at now
    std::string name;
    mat4 m = mat4(1.0f);
    std::vector<uint32_t> c;  // children
    int num = -1;             // or call index, can find it use model.nodes[num]
    std::string type = "node";
};

struct Camera : public Node {
    float znear = 0.0;
    float zfar = 0.0;

    // perspective
    double aspectRatio = 0.0;  // min > 0
    double yfov = 0.0;         // required. min > 0
    // orthographic
    double xmag = 0.0;   // required. must not be zero.
    double ymag = 0.0;   // required. must not be zero.

    vec3 w; // forward
    vec3 e; // location of eye(cam)
    vec3 v; // cam base up
    vec3 u; // cam base right

    Camera() {};
    Camera(const Node& other) : Node(other) {};
};

using Material = tinygltf::Material;

struct Ray {
    vec3 o;
    vec3 d;
};

struct Interaction { // now just triangle
    const Triangle* tri;
    vec3 bary; // barycentric
    vec3 p;
    float tMin;

    bool Intersect(const Ray& r, const std::vector<Triangle>& tris, float tMin, float tMax);
};

struct Screen {
    uint16_t w = 0;
    uint16_t h = 0;
};

inline void PrintVec(vec3 v)
{
    for (int i = 0; i < 3; i++) {
        if (std::fabs(v[i]) < 1e-6f) {
            std::cout << 0 << " ";
        } else {
            std::cout << v[i] << " ";
        }   
    }
    std::cout << std::endl;
}

inline void PrintMat4(mat4 m)
{
    /*for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (std::fabs(m[j][i]) < 1e-6f) {
                std::cout << 0 << " ";
            } else {
                std::cout << m[j][i] << " ";
            }
        }
        std::cout << std::endl;
    }*/

    // be care glm is col major
    std::cout << glm::to_string(m) << std::endl;
}

inline vec3 GetTriNormalizeByBary(const Triangle* tri, const vec3& bary)
{
    const auto a = tri->vts[0];
    const auto b = tri->vts[1];
    const auto c = tri->vts[2];
    return normalize(bary[0] * a->normal + bary[1] * a->normal + bary[2] * a->normal);
}
}
#endif // !PUB