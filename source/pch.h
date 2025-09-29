#pragma once
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
#include <numeric>
#include <thread>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <queue>

static float Gamma = 2.2; // gamma to linear
static float GammaInv = 1 / Gamma;
static float ShadowEpsilon = 1e-5;
static float FloatEpsilon = 1e-6;
static float Pi = 3.14159265358979323846;
static float InvPi = 0.31830988618379067154;
static float Inv2Pi = 0.15915494309189533577;
static float Inv4Pi = 0.07957747154594766788;
static float PiOver2 = 1.57079632679489661923;
static float PiOver4 = 0.78539816339744830961;
static float Sqrt2 = 1.41421356237309504880;

constexpr float floatMax = std::numeric_limits<float>::infinity();

#define GLM_ENABLE_EXPERIMENTAL
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
namespace gltf = tinygltf;

inline void PrintVec(const vec3& v)
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

inline void PrintMat4(const mat4& m)
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