#pragma once
#include "public.h"
namespace ry {
class RGBSpectrum {
public:
    RGBSpectrum(float v = 0.f) : c{ v } {}
    RGBSpectrum(const vec3& v) : c(v) {}

    RGBSpectrum& operator+=(const RGBSpectrum& c2) {
        c = c + c2.c;
        return *this;
    }
    RGBSpectrum& operator+=(const vec3& c2) {
        c = c + c2;
        return *this;
    }
    RGBSpectrum operator*(const float c2) const {
        return RGBSpectrum(c * c2);
    }
    RGBSpectrum operator*(const RGBSpectrum& c2) const {
        return RGBSpectrum(c * c2.c);
    }
    RGBSpectrum& operator*=(const RGBSpectrum& c2) {
        c = c * c2.c;
        return *this;
    }
    RGBSpectrum operator+(const RGBSpectrum& c2) const {
        return RGBSpectrum(c + c2.c);
    }
    RGBSpectrum operator/(float c2) const {
        return RGBSpectrum(c / c2);
    }
    RGBSpectrum operator-(const RGBSpectrum& c2) const {
        return RGBSpectrum(c - c2.c);
    }

    inline bool IsBlack() const {
        for (int i = 0; i < 3; ++i) {
            if (c[i] != 0.) { return false; }
        }
        return true;
    }
    inline float Luminance() const {
        return 0.2126 * c.x + 0.7152 * c.y + 0.0722 * c.z;
    }
    vec3 c;
};
using Spectrum = RGBSpectrum;

struct Triangle {
    vec3 v0, v1, v2;
    vec3 n0, n1, n2;
    vec2 uv0, uv1, uv2;
    int material;
    // vec4 color;
    // vec3 c; // centroid
};

struct Camera {
    char type; // 0 = perspective, other = orthographic
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

    mat4 projMatrix; // camera to clip
    mat4 clipToCamera; // proj inverse

    mat4 viewMatrix; // inverse(node.m)
    mat4 projView;

    mat4 m; // trans mat
};

struct Light {
    float area = 0.0f;
    float emissiveStrength;
    Spectrum I; // or emissiveFactor
    std::vector<uint32_t> tIdxs; // tris idxs
};

struct Ray {
    vec3 o, d;
};

struct Interaction {
    float tMin;
    float tMax;

    vec3 bary;     // barycentric
    vec3 p;        // hit point
    vec3 normal;   // of hit face
;
    int hit = -1;
};

struct Material {
    
};

class Model {
public:
	Model() {};
    std::vector<Triangle>& GetTriangles() { return tris; }
    const std::vector<Camera>& GetCameras() const { return cams; }
    void AddCamera(const Camera& c) { cams.push_back(c); }
private:
    std::vector<Triangle> tris;
    std::vector<Camera> cams;
};
}