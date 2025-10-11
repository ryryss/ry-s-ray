#include "pch.h"
namespace ry {
struct PixelInfo {
    vec3 color;
    vec3 normal;
    float depth;
    // float roughness;
    vec3 albedo;
    vec2 motion;
};

struct TemporalInfo {
    vec3 M1;      // first moment (mean)
    vec3 M2;      // second moment (mean of squares)
    float age;    // how many frames contributed (can use to alpha fix)
};

class Denoiser {
public:
    virtual ~Denoiser() {};
    Denoiser(const ivec2& r)
        : radius(r), invRadius(ivec2(1 / radius.x, 1 / radius.y)) {}
    virtual vec3 Denoise(int x, int y) = 0;
    virtual void ReSize(int w, int h) { width = w; height = h; }
    virtual void AfterDenoise() {};
    virtual std::vector<PixelInfo>* GetGBuffer() { return &gBuffer; };
protected:
    const ivec2 radius, invRadius;
    int width, height;
    std::vector<PixelInfo> gBuffer;
};

class BilateralDenoiser : public Denoiser {
public:
    BilateralDenoiser(const ivec2& r) : Denoiser(r) {}

    vec3 Denoise(int x, int y) override;

    inline void ReSize(int w, int h) override
    {
        width = w;
        height = h;
        gBuffer.clear();
        gBuffer.resize(w * h);
    }
private:
    float sigmaSpatial = 2.0f;
    float sigmaColor = 0.3f;
    float sigmaNormal = 0.6f;
    float sigmaAlbedo = 0.5f;
    float sigmaDepth = 0.1f;
};

class TemporalDenoiser : public Denoiser {
public:
    TemporalDenoiser(const ivec2& r) : Denoiser(r) {}

    vec3 Denoise(int x, int y) override;
    inline void AfterDenoise() override {
        prevGBuffer = gBuffer;
        prevTempBuffer = tempBuffer;
    };
    inline void ReSize(int w, int h) override
    {
        width = w;
        height = h;
        gBuffer.clear();
        gBuffer.resize(w * h);
        prevGBuffer.clear();
        prevGBuffer.resize(w * h);
        tempBuffer.clear();
        tempBuffer.resize(w * h);
        prevTempBuffer.clear();
        prevTempBuffer.resize(w * h);
    }
private:
    std::vector<PixelInfo> prevGBuffer;
    std::vector<TemporalInfo> tempBuffer;
    std::vector<TemporalInfo> prevTempBuffer;

    float alpha = 0.15f;           // temporal blending factor for new sample
    float normalThresh = 0.90f;    // dot(N_cur, N_prev) threshold for history acceptance
    float depthThresh = 0.02f;     // depth absolute threshold (in depth units)
    float clampK = 1.5f;           // clamp factor (k * sigma)
    int atrousIterations = 3;      // A-Trous iterations for spatial filter
    float phiColor = 10.0f;        // bilateral: color sensitivity (higher -> preserve edges)
    float phiNormal = 128.0f;      // normal sensitivity (scaled)
    float phiDepth = 2.0f;         // depth sensitivity
};
}