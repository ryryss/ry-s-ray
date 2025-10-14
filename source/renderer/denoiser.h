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
    virtual vec3 Denoise(int x, int y, const std::vector<PixelInfo>& gBuffer) = 0;
    virtual void ReSize(int w, int h) { width = w; height = h; }
    virtual void AfterDenoise() {};
protected:
    const ivec2 radius, invRadius;
    int width, height;
};

class BilateralDenoiser : public Denoiser {
public:
    BilateralDenoiser(const ivec2& r) : Denoiser(r) {}

    vec3 Denoise(int x, int y, const std::vector<PixelInfo>& gBuffer) override;
private:
    float sigmaSpatial = 2.0f;
    float sigmaColor = 0.3f;
    float sigmaNormal = 0.6f;
    float sigmaAlbedo = 0.5f;
    float sigmaDepth = 0.1f;
};

class AtrousDenoiser : public Denoiser {
public:
    AtrousDenoiser(const ivec2& r) : Denoiser(r) {}

    vec3 Denoise(int x, int y, const std::vector<PixelInfo>& gBuffer) override;
    inline void ReSize(int w, int h) override {
        width = w;
        height = h;
        ping.clear();
        ping.resize(w * h);
        pong.clear();
        pong.resize(w * h);
    }
    inline std::vector<vec3>& GetPing() {
        return ping;
    }
    inline void SetTteration(int i) {
        iteration = i;
        step = 1; // reset step
    }
    void SwapPingPong() {
        std::swap(ping, pong);
        step *= 2;
    }

private:
    int iteration = 3;
    int step = 1;
    std::vector<vec3> ping;
    std::vector<vec3> pong;

    const float sigmaColor = 3.0f;
    const float sigmaNormal = 0.1f;
    const float sigmaDepth = 0.02f;
    const float sigmaAlbedo = 0.2f;
};

class TemporalDenoiser : public Denoiser {
public:
    TemporalDenoiser(const ivec2& r) : Denoiser(r) {}

    vec3 Denoise(int x, int y, const std::vector<PixelInfo>& gBuffer) override;
    inline void AfterDenoise() override {
        prevTempBuffer = tempBuffer;
    };
    inline void KeepGBuffer(const std::vector<PixelInfo>& gBuffer) {
        prevGBuffer = gBuffer;
    }
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