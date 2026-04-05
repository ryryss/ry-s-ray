#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/random.hpp>

namespace ry {
constexpr float GammaLinear = 2.2f; // gamma to linear
constexpr float GammaSRGB = 1.f / GammaLinear;
constexpr float ShadowEpsilon = 1e-5;
constexpr float FloatEpsilon = 1e-6;
constexpr float Pi = 3.14159265358979323846;
constexpr float InvPi = 0.31830988618379067154;
constexpr float Inv2Pi = 0.15915494309189533577;
constexpr float Inv4Pi = 0.07957747154594766788;
constexpr float PiOver2 = 1.57079632679489661923;
constexpr float PiOver4 = 0.78539816339744830961;
constexpr float Sqrt2 = 1.41421356237309504880;

// constexpr float floatMax = std::numeric_limits<float>::infinity();
using mat3 = glm::mat3;
using mat4 = glm::mat4;
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
/*struct vec2 {
    float x, y;

    vec2() : x(0), y(0){}
    vec2(float x) : x(x), y(x) {}
    vec2(float x, float y) : x(x), y(y){}
};

//use 16bytes vec3 for cuda
struct alignas(16) vec3 {
    float x, y, z, w;

    vec3() : x(0), y(0), z(0), w(0) {}
    vec3(float x) : x(x), y(x), z(x), w(0) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z), w(0) {}
};
static_assert(sizeof(vec3) == 16, "vec3 must be 16 bytes");

struct alignas(16) vec4 {
    float x, y, z, w;

    vec4() : x(0), y(0), z(0), w(0) {}
    vec4(float x) : x(x), y(x), z(x), w(x) {}
    vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    vec4(vec3 v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
};
static_assert(sizeof(vec4) == 16, "vec4 must be 16 bytes");*/
}