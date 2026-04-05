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

#ifndef USE_CUDA
using mat3 = glm::mat3;
using mat4 = glm::mat4;
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
#else

#ifdef __CUDACC__
#define HD __host__ __device__
#else
#define HD
#endif

struct vec2;
struct vec3;
struct vec4;
struct mat3;
struct mat4;
/********************************
            vec2
********************************/
struct vec2 {
    float x, y;

    HD vec2() : x(0), y(0){}
    HD vec2(float x) : x(x), y(x) {}
    HD vec2(float x, float y) : x(x), y(y){}
};
/********************************
            vec3
********************************/
//use 16bytes vec3 for cuda
struct alignas(16) vec3 {
    float x, y, z, w;

    HD vec3() : x(0), y(0), z(0), w(0) {}
    HD vec3(float x) : x(x), y(x), z(x), w(0) {}
    HD vec3(float x, float y, float z) : x(x), y(y), z(z), w(0) {}
    HD vec3(const vec4& v);

    HD float& operator[](int i) {
        return *(&x + i);
    }

    HD const float& operator[](int i) const {
        return *(&x + i);
    }
};
static_assert(sizeof(vec3) == 16, "vec3 must be 16 bytes");

HD inline vec3 operator+(const vec3& a, const vec3& b)
{
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

HD inline vec3 operator-(const vec3& a, const vec3& b)
{
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

HD inline vec3 operator*(const vec3& a, float b)
{
    return vec3(a.x * b, a.y * b, a.z * b);
}

HD inline vec3 operator*(const vec3& a, const vec3& b) {
    return vec3(
        a.x * b.x,
        a.y * b.y,
        a.z * b.z);
}

HD inline vec3 operator/(const vec3& a, float b)
{
    return vec3(a.x / b, a.y / b, a.z / b);
}

HD inline vec3 operator-(const vec3& v)
{
    return vec3(-v.x, -v.y, -v.z);
}

HD inline float dot(const vec3& a, const vec3& b)
{
    return a.x * a.x + a.y * b.y + a.z * b.z;
}

HD inline vec3 cross(const vec3& a, const vec3& b)
{
    return vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x);
}

HD inline float length(const vec3& v)
{
    return sqrtf(dot(v, v));
}

HD inline vec3 normalize(const vec3& v)
{
    return v / length(v);
}
/********************************
            vec4
********************************/
struct alignas(16) vec4 {
    float x, y, z, w;

    HD vec4() : x(0), y(0), z(0), w(0) {}
    HD vec4(float x) : x(x), y(x), z(x), w(x) {}
    HD vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    HD vec4(vec3 v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
};
static_assert(sizeof(vec4) == 16, "vec4 must be 16 bytes");

HD inline vec3::vec3(const vec4& v) : x(v.x), y(v.y), z(v.z), w(0) {}

HD inline vec4 operator+(const vec4& a, const vec4& b)
{
    return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

HD inline vec4 operator-(const vec4& a, const vec4& b)
{
    return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

HD inline vec4 operator*(const vec4& a, float b)
{
    return vec4(a.x * b, a.y * b, a.z * b, a.w * b);
}

HD inline vec4 operator/(const vec4& a, float b)
{
    return vec4(a.x / b, a.y / b, a.z / b, a.w / b);
}

HD inline float dot(const vec4& a, const vec4& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
/********************************
            mat3
********************************/
struct mat3 {
    vec3 c0, c1, c2; // clumn major order

    HD mat3(float s = 1.0) {
        c0 = vec3(s, 0, 0);
        c1 = vec3(0, s, 0);
        c2 = vec3(0, 0, s);
    }

    HD mat3(const mat4& m);

    HD mat3(const vec3& c0, const vec3& c1, const vec3& c2)
        : c0(c0), c1(c1), c2(c2) {}
};
// mat3 * vec3
HD inline vec3 operator*(const mat3& m, const vec3& v)
{
    return vec3(
        m.c0.x * v.x + m.c1.x * v.y + m.c2.x * v.z,
        m.c0.y * v.x + m.c1.y * v.y + m.c2.y * v.z,
        m.c0.z * v.x + m.c1.z * v.y + m.c2.z * v.z);
}

HD inline mat3 operator*(const mat3& a, const mat3& b)
{
    return mat3(
        a * b.c0,
        a * b.c1,
        a * b.c2);
}

HD inline mat3 transpose(const mat3& m)
{
    return mat3(
        vec3(m.c0.x, m.c1.x, m.c2.x),
        vec3(m.c0.y, m.c1.y, m.c2.y),
        vec3(m.c0.z, m.c1.z, m.c2.z));
}

HD inline mat3 inverse(const mat3& m) {
    const vec3& a = m.c0;
    const vec3& b = m.c1;
    const vec3& c = m.c2;

    vec3 r0 = cross(b, c);
    vec3 r1 = cross(c, a);
    vec3 r2 = cross(a, b);

    float invDet = 1.0f / dot(r2, c);

    mat3 inv;
    inv.c0 = r0 * invDet;
    inv.c1 = r1 * invDet;
    inv.c2 = r2 * invDet;

    return inv;
}
/********************************
            mat4
********************************/
struct mat4 {
    vec4 c0, c1, c2, c3; // clumn major order

    HD mat4(const vec4& c0, const vec4& c1,
        const vec4& c2, const vec4& c3)
        : c0(c0), c1(c1), c2(c2), c3(c3) {}

    HD mat4(float s = 1.0f) {
        c0 = vec4(s, 0, 0, 0);
        c1 = vec4(0, s, 0, 0);
        c2 = vec4(0, 0, s, 0);
        c3 = vec4(0, 0, 0, s);
    }

    HD mat4(const double* m) {
        c0 = vec4(m[0], m[1], m[2], m[3]);
        c1 = vec4(m[4], m[5], m[6], m[7]);
        c2 = vec4(m[8], m[9], m[10], m[11]);
        c3 = vec4(m[12], m[13], m[14], m[15]);
    }

    HD vec4& operator[](int i) {
        return (&c0)[i];
    }

    HD const vec4& operator[](int i) const
    {
        return (&c0)[i];
    }
};

// mat3 construction
HD inline mat3::mat3(const mat4& m) {
    c0 = vec3(m.c0);
    c1 = vec3(m.c1);
    c2 = vec3(m.c2);
}

// mat4 * vec4
HD inline vec4 operator*(const mat4& m, const vec4& v)
{
    return vec4(
        m.c0.x * v.x + m.c1.x * v.y + m.c2.x * v.z + m.c3.x * v.w,
        m.c0.y * v.x + m.c1.y * v.y + m.c2.y * v.z + m.c3.y * v.w,
        m.c0.z * v.x + m.c1.z * v.y + m.c2.z * v.z + m.c3.z * v.w,
        m.c0.w * v.x + m.c1.w * v.y + m.c2.w * v.z + m.c3.w * v.w);
}

HD inline mat4 operator*(const mat4& a, const mat4& b)
{
    return mat4(
        a * b.c0,
        a * b.c1,
        a * b.c2,
        a * b.c3);
}

HD inline mat4 translate(const mat4& m, const vec3& t)
{
    mat4 r = m;

    r.c3 = m.c0 * t.x + m.c1 * t.y +
           m.c2 * t.z + m.c3;
    return r;
}

HD inline mat4 scale(const mat4& m, const vec3& s)
{
    mat4 r;

    r.c0 = m.c0 * s.x;
    r.c1 = m.c1 * s.y;
    r.c2 = m.c2 * s.z;
    r.c3 = m.c3;
    return r;
}
/********************************
            quat
********************************/
struct quat {
    float w, x, y, z;

    HD quat() : w(1), x(0), y(0), z(0) {}

    HD quat(float w_, float x_, float y_, float z_)
        : w(w_), x(x_), y(y_), z(z_) {}
};

HD inline quat normalize(const quat& q) {
    float len = sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    return quat(q.w / len, q.x / len, q.y / len, q.z / len);
}

HD inline quat operator*(const quat& a, const quat& b) {
    return quat(
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w
    );
}


HD inline mat4 toMat4(const quat& q) {
    float x = q.x, y = q.y, z = q.z, w = q.w;

    float xx = x * x;
    float yy = y * y;
    float zz = z * z;
    float xy = x * y;
    float xz = x * z;
    float yz = y * z;
    float wx = w * x;
    float wy = w * y;
    float wz = w * z;

    mat4 r;

    r.c0 = vec4(1 - 2 * (yy + zz),
        2 * (xy + wz),
        2 * (xz - wy),
        0);

    r.c1 = vec4(2 * (xy - wz),
        1 - 2 * (xx + zz),
        2 * (yz + wx),
        0);

    r.c2 = vec4(2 * (xz + wy),
        2 * (yz - wx),
        1 - 2 * (xx + yy),
        0);

    r.c3 = vec4(0, 0, 0, 1);

    return r;
}
#endif
}