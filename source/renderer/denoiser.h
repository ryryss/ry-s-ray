#include "pch.h"
namespace ry {
struct PixelInfo {
    vec3 color;
    vec3 normal;
    float depth;
    // float roughness;
    vec3 position;
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
    Denoiser(const ivec2& r) : radius(r) {}
    virtual void Denoise(int x, int y, const PixelInfo* input, vec4* output) = 0;
    virtual void ReSize(int w, int h) { width = w; height = h; }
protected:
    const ivec2 radius;
    int width, height;
    const PixelInfo* gBuffer;
};

class AtrousDenoiser : public Denoiser {
public:
    AtrousDenoiser(const ivec2& r) : Denoiser(r) {}

    void Denoise(int x, int y, const PixelInfo* gBuffer, vec4* output) override;
    inline void ReSize(int w, int h) override {
        width = w;
        height = h;
        ping.clear();
        ping.resize(w * h);
        pong.clear();
        pong.resize(w * h);
    }

private:
    vec4 AtrousDenoise(uint16_t x, uint16_t y);

    int iteration = 5;
    int step = 1;
    std::vector<vec3> ping;
    std::vector<vec3> pong;

    const float sigmaColor0 = 5.0f;
    float sigmaColor = 40.0f;
    float sigmaNormal = 0.05f;
    float sigmaPosition = 0.2f;
};

class TemporalDenoiser : public Denoiser {
public:
    TemporalDenoiser(const ivec2& r) : Denoiser(r) {}

    void Denoise(int x, int y, const PixelInfo* input, vec4* output) override;

    inline void ReSize(int w, int h) override {
        width = w;
        height = h;
        prevGBuffer.clear();
        prevGBuffer.resize(w * h);
        tempBuffer.clear();
        tempBuffer.resize(w * h);
        prevTempBuffer.clear();
        prevTempBuffer.resize(w * h);
    }
private:
    vec4 TemporalDenoise(uint16_t x, uint16_t y);

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