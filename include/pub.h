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
#define M_PI 3.14159265358979323846f

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

struct Vertex {
    vec3 pos;
    vec4 color = vec4(1.0);
    vec3 normal;
    vec2 uv;
    int i;
};

struct Triangle {
    vec4 color;
    uint32_t idx[3]; // use for  model vertices
    vec3 bary; // barycentric
    int material;
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

class RGBSpectrum {
public:
    RGBSpectrum(float v = 0.f) : c{ v } {}
    RGBSpectrum(vec3 v) : c(v) {}

    RGBSpectrum& operator+=(const RGBSpectrum& c2) {
        c += c2.c;
        return *this;
    }
    RGBSpectrum& operator+(const RGBSpectrum& c2) {
        return RGBSpectrum(c + c2.c);
    }
    RGBSpectrum& operator/(float c2) {
        return RGBSpectrum(c / c2);
    }
    vec3 c;
};
using Spectrum = RGBSpectrum;

class Light : public Node {
public:
    Light() {};
    Light(const Node& other) : Node(other) {};

    float area = 0.0f;
    float emissiveStrength;
    Spectrum I; // or emissiveFactor
    std::vector<Triangle> tris;
};

using Material = tinygltf::Material;

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

}
#endif // !PUB