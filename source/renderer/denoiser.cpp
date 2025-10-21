#include "denoiser.h"
#include "sampler.hpp"
#include "task.h"
using namespace ry;
using namespace std;

void AtrousDenoiser::Denoise(int x, int y, const PixelInfo* input, vec4* output)
{
    gBuffer = input;
    step = 1;
    for (int i = 0; i < width * height; i++) {
        // use output of the previous stage as input
        ping[i] = vec3(output[i]);
    }

    auto& t = Task::GetInstance();
    for (int i = 0; i < iteration; i++) {
        sigmaColor = sigmaColor0 * powf(0.5f, i);
        step = 1 << i;
        t.Parallel2D(width, height, 32, [this, &output](uint16_t x, uint16_t y) {
            output[y * width + x] = AtrousDenoise(x, y);
        });
        swap(ping, pong);
    }
}

vec4 AtrousDenoiser::AtrousDenoise(uint16_t x, uint16_t y)
{
    int p = y * width + x;
    const PixelInfo& center = gBuffer[p];
    const auto& centerColor = ping[p]; // must use new color

    Spectrum sumColor(0);
    float sumWeight = 0.0f;
    for (int dy = -radius.x; dy <= radius.x; ++dy) {
        for (int dx = -radius.x; dx <= radius.x; ++dx) {
            int sx = x + dx * step;
            int sy = y + dy * step;
            if (sx < 0 || sy < 0 || sx >= width || sy >= height) { continue; }
            int q = sy * width + sx;
            const PixelInfo& nb = gBuffer[q];
            const auto& nbColor = ping[q]; // must use new color
            // color weight
            float wRt = 1.0f;
            if (step > 1) {
                float dc = glm::length(centerColor.c - nbColor.c);
                wRt = expf(-(dc * dc) / (sigmaColor * sigmaColor));
            }
            // normal weight
            float dn = glm::length(center.normal - nb.normal);
            float wN = expf(-(dn * dn) / (sigmaNormal * sigmaNormal));
            // position weight
            float dxp = glm::length(center.position - nb.position) * 0.1;
            float wX = expf(-(dxp * dxp) / (sigmaPosition * sigmaPosition));
            // albedo weight
            float da = glm::length(center.albedo - nb.albedo);
            float wA = expf(-(da * da) / (sigmaAlbedo * sigmaAlbedo));

            float w = wRt * wN * wX * wA;
            float kWeight = kernel[dx + radius.x] * kernel[dy + radius.x]; // / kSum2;
            sumColor += nbColor * w * kWeight;
            sumWeight += w * kWeight;
        }
    }
    pong[p] = sumWeight > 1e-5f ? sumColor / sumWeight : center.color;
    return { pong[p].c, 1.0 };
}

void Spatiotemporal::Denoise(int x, int y, const PixelInfo* input, vec4* output)
{
    gBuffer = input;
    auto& t = Task::GetInstance();
    t.Parallel2D(width, height, 32, [this](uint16_t x, uint16_t y) {
        TemporalAccumulation(x, y);
    });
#ifdef DEBUG
    static_assert(std::is_trivially_copyable_v<PixelInfo>, "PixelInfo must be trivially copyable");
#endif // DEBUG
    // memcpy(prevGBuffer.data(), gBuffer, prevGBuffer.size());
    t.Parallel2D(width, height, 32, [&output, this](uint16_t x, uint16_t y) {
        uint32_t idx = y * width + x;
        prevGBuffer[idx] = gBuffer[idx];
        // output[idx] = vec4(ping[idx].c, 1.0);
    });

    for (int i = 0; i < iteration; i++) {
        sigmaColor = sigmaColor0 * powf(0.5f, i);
        step = 1 << i;
        t.Parallel2D(width, height, 32, [this, &output](uint16_t x, uint16_t y) {
            output[y * width + x] = SpatialFilter(x, y);
        });
        swap(ping, pong);
        swap(varPing, varPong);

        if (i == 1) {
            // we output the ltered color from the rst wavelet iteration
            // as our color history used to temporally integrate with future frames
            for (uint16_t i = 0; i < height; i++) {
                for (uint16_t j = 0; j < width; j++) {
                    uint32_t idx = i * width + j;
                    prevTemporalBuffer[idx] = temporalBuffer[idx];
                    prevTemporalBuffer[idx].color = pong[idx];
                }
            }
        }
    }
}

ivec2 Spatiotemporal::ReprojectPrevPixel(const vec2& prevCoord)
{
    vec2 prevUV = prevCoord / vec2(width, height);
    const auto& prevPixel = SampleNearest<ivec2>(prevUV.x, prevUV.y, width, height,
        [&](int x, int y) { return ivec2(x, y); }
    );
    return prevPixel;
}

void Spatiotemporal::TemporalAccumulation(uint16_t x, uint16_t y)
{
    int idx = y * width + x;
    const auto& currPixel = gBuffer[idx];
    const auto& currColor = currPixel.color;
    float L = currColor.Luminance();
    float L2 = L * L;
    auto& currTemporal = temporalBuffer[idx];

    auto oldCoord = ReprojectPrevPixel(vec2(x, y) + currPixel.motion);
    if (oldCoord.x >= width || oldCoord.x < 0 || oldCoord.y > height || oldCoord.y <0) {
        currTemporal.M = L;
        currTemporal.M2 = L2;
        currTemporal.age = 1.0f;
        currTemporal.color = currPixel.color.c;
    }
    int oldIdx = oldCoord.y * width + oldCoord.x;
    const auto& prevTemporal = prevTemporalBuffer[oldIdx];
    const auto& prevPixel = prevGBuffer[oldIdx];
    // Where our temporal history is limited (<4 frames aer a disocclusion), 
    // we instead estimate the variance σ2i spatially
    if (prevTemporal.age <= 4) {
        currTemporal.var = 1.0f; // TODO
    }
    // To improve image quality under motion we resample Ci−1 by
    // using a 2 × 2 tap bilinear lter
    float newM = 1.0;
    float newM2 = 1.0;
    float ndot = dot(currPixel.normal, prevPixel.normal);
    float dz = abs(currPixel.depth - prevPixel.depth);
    if (ndot > 0.9f && dz < 0.05f) {
        newM = glm::mix(L, prevTemporal.M, alpha);
        newM2 = glm::mix(L2, prevTemporal.M2, alpha);
        currTemporal.age = min(prevTemporal.age + 1.0f, 100.0f);
        currTemporal.color = glm::mix(prevTemporal.color.c, currColor.c, alpha);
    } else {
        newM = L;
        newM2 = L2;
        currTemporal.age = 1.0f;
        currTemporal.color = currColor;
    }
    // using the simple formula
    float var = max(newM2 - newM * newM, 0.0f);

    currTemporal.M = newM;
    currTemporal.M2 = newM2;
    currTemporal.var = var;

    // set spatial input
    ping[idx] = currTemporal.color;
}

vec4 Spatiotemporal::SpatialFilter(uint16_t x, uint16_t y)
{
    int p = y * width + x;
    const auto& center = gBuffer[p];
    const auto& centerColor = ping[p]; // must use new color

    Spectrum sumColor(0);
    float sumWeight = 0.0f;
    float var = 0.0;
    for (int dy = -radius.x; dy <= radius.x; ++dy) {
        for (int dx = -radius.x; dx <= radius.x; ++dx) {
            int sx = x + dx * step;
            int sy = y + dy * step;
            if (sx < 0 || sy < 0 || sx >= width || sy >= height) { continue; }
            int q = sy * width + sx;
            const PixelInfo& nb = gBuffer[q]; // must use new color
            const auto& nbColor = ping[q];
            // color weight
            float wRt = 1.0f;
            if (step > 1) {
                float dc = glm::length(centerColor.c - nbColor.c);
                wRt = expf(-(dc * dc) / (sigmaColor * sigmaColor));
            }
            // normal weight
            float dn = glm::length(center.normal - nb.normal);
            float wN = expf(-(dn * dn) / (sigmaNormal * sigmaNormal));
            // position weight
            float dxp = glm::length(center.position - nb.position) * 0.1;
            float wX = expf(-(dxp * dxp) / (sigmaPosition * sigmaPosition));

            float w = wRt * wN * wX;
            float kWeight = kernel[dx + radius.x] * kernel[dy + radius.x]; // / kSum2;
            sumColor += nbColor * w * kWeight;
            sumWeight += w * kWeight;

            float qvar = varPing[q];
            var += kWeight * kWeight * w * w * qvar;
        }
    }

    varPong[p] = max(var / (sumWeight * sumWeight), 0.0f);
    pong[p] = sumWeight > 1e-5f ? sumColor / sumWeight : center.color;
    return { pong[p].c, 1.0 };
}
