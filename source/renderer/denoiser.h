#include "pch.h"
#include "Spectrum.hpp"
namespace ry {
struct PixelInfo {
    Spectrum color;
    vec3 normal;
    float depth;
    // float roughness;
    vec3 position;
    vec2 motion;
    vec3 albedo;
};

struct TemporalInfo {
    float M = 1.0;     // first moment (mean)
    float M2 = 1.0;    // second moment (mean of squares)
    float age = 1.0;   // how many frames contributed (can use to alpha fix)
    float var = 0.0;   // variance
    Spectrum color;
};

class Denoiser {
public:
    virtual ~Denoiser() {};
    Denoiser(const ivec2& r) : radius(r) {}
    virtual void Denoise(int x, int y, const PixelInfo* input, vec4* output) = 0;
    virtual void ReSize(int w, int h) { width = w; height = h; }
protected:
    ivec2 radius;
    int width, height;
    const PixelInfo* gBuffer;
};

// Edge-Avoiding À-Trous Wavelet Transform for fast Global Illumination Filtering(2010)
class AtrousDenoiser : public Denoiser {
public:
    AtrousDenoiser(const ivec2& r) : Denoiser(r) { radius = ivec2(kernelSize / 2); }

    void Denoise(int x, int y, const PixelInfo* gBuffer, vec4* output) override;
    inline void ReSize(int w, int h) override {
        width = w;
        height = h;
        ping.clear();
        ping.resize(w * h);
        pong.clear();
        pong.resize(w * h);
    }
protected:
    int iteration = 5;
    int step = 1;
    std::vector<Spectrum> ping;
    std::vector<Spectrum> pong;

    const float sigmaColor0 = 2.0f;
    float sigmaColor = 5.0f;
    float sigmaNormal = 0.3f;
    float sigmaPosition = 0.1f;
    float sigmaAlbedo = 0.08f;

    const static inline int kernelSize = 5;
    const static inline float kernel[kernelSize] = { 1.f / 16, 1.f / 4, 3.f / 8, 1.f / 4, 1.f / 16 };
    const static inline float kSum = 1.f / 16 + 1.f / 4 + 3.f / 8 + 1.f / 4 + 1.f / 16;
    const static inline float kSum2 = kSum * kSum;
private:
    vec4 AtrousDenoise(uint16_t x, uint16_t y);
};

// Spatiotemporal Variance-Guided Filtering: Real-Time
// Reconstruction for Path - Traced Global Illumination
class Spatiotemporal : public AtrousDenoiser {
public:
    Spatiotemporal(const ivec2& r) : AtrousDenoiser(r) {}

    void Denoise(int x, int y, const PixelInfo* input, vec4* output) override;

    inline void ReSize(int w, int h) override {
        width = w;
        height = h;
        AtrousDenoiser::ReSize(w, h);
        temporalBuffer.clear();
        temporalBuffer.resize(w * h);
        prevTemporalBuffer.clear();
        prevTemporalBuffer.resize(w * h);
        prevGBuffer.clear();
        prevGBuffer.resize(w * h);
        varPing.clear();
        varPing.resize(w * h);
        varPong.clear();
        varPong.resize(w * h);
    }
private:
    vec4 SpatiotemporalDenoise(uint16_t x, uint16_t y);
    void TemporalAccumulation(uint16_t x, uint16_t y);
    vec4 SpatialFilter(uint16_t x, uint16_t y);

    std::vector<TemporalInfo> temporalBuffer;
    std::vector<TemporalInfo> prevTemporalBuffer;
    std::vector<PixelInfo> prevGBuffer;
    std::vector <float> varPing;
    std::vector <float> varPong;
    const static inline float alpha = 0.2;
};
}