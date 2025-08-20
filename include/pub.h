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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

namespace ry {
using vec3 = glm::vec3; // can ez change mat lib 
using mat4 = glm::mat4;

struct Vertex {
    vec3 position;
    vec3 color;
};

struct Triangle {
    vec3 pos[3];
    vec3 color[3];
};

struct Node {
    Node* p = nullptr; // parent, no where use this at now
    std::string name;
    mat4 m = mat4(1.0f);
    std::vector<uint32_t> c; // children
};

struct Camera
{
    std::string type = "perspective";
    std::string name;
    mat4 m = mat4(1.0f);

    float znear = 0.0;
    float zfar = 0.0;

    // perspective
    double aspectRatio{ 0.0 };  // min > 0
    double yfov{ 0.0 };         // required. min > 0
    // orthographic
    double xmag{ 0.0 };   // required. must not be zero.
    double ymag{ 0.0 };   // required. must not be zero.
};

struct Screen
{
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
    // col major
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
    std::cout << glm::to_string(m) << std::endl;
}

}
#endif // !PUB