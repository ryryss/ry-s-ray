#include "denoiser.h"
#include "sampler.hpp"
#include "task.h"
using namespace ry;
using namespace std;

void TemporalDenoiser::Denoise(int x, int y, const PixelInfo* input, vec4* output)
{
    gBuffer = input;
    auto& t = Task::GetInstance();
    t.Parallel2D(width, height, 32, [this, &output](uint16_t x, uint16_t y) {
        output[y * width + x] = TemporalDenoise(x, y);
    });
    prevTempBuffer = tempBuffer;
    copy(input, input + width * height, prevGBuffer.begin());
}

vec4 TemporalDenoiser::TemporalDenoise(uint16_t x, uint16_t y)
{
    int idx = y * width + x;
    const PixelInfo& curPixel = gBuffer[idx];
    // 1) compute previous pixel location via motion vector
    vec2 prevPixel = vec2(x + 0.5, y + 0.5) + curPixel.motion;
    vec2 prevUV = prevPixel / vec2(width, height);
    if (prevUV.x < 0.0 || prevUV.x > 1.0 || prevUV.y < 0.0 || prevUV.y > 1.0) {
        tempBuffer[idx].M1 = curPixel.color;
        tempBuffer[idx].M2 = curPixel.color * curPixel.color;
        tempBuffer[idx].age = 1.0f;
        return vec4(curPixel.color, 1.0);
    }
    // 2) sample previous temporal moments and previous GBuffer normal/depth
    vec3 prevM1 = SampleBilinear<vec3>(prevUV.x, prevUV.y, width, height,
        [&](int x, int y) { return prevTempBuffer[y * width + x].M1; }
    );
    vec3 prevM2 = SampleBilinear<vec3>(prevUV.x, prevUV.y, width, height,
        [&](int x, int y) { return prevTempBuffer[y * width + x].M2; }
    );
    vec3 prevNormal = SampleNearest<vec3>(prevUV.x, prevUV.y, width, height,
        [&](int x, int y) { return prevGBuffer[y * width + x].normal;}
    );
    float prevDepth = SampleNearest<float>(prevUV.x, prevUV.y, width, height,
        [&](int x, int y) { return prevGBuffer[y * width + x].depth; }
    );
    float prevAge = SampleBilinear<float>(prevUV.x, prevUV.y, width, height,
        [&](int x, int y) { return prevTempBuffer[y * width + x].age; }
    );
    // 3) history acceptance (normal & depth check)
    bool historyAccepted = true;
    if (dot(curPixel.normal, prevNormal) < normalThresh) { historyAccepted = false; }
    if (abs(curPixel.depth - prevDepth) > depthThresh) { historyAccepted = false; }

    vec3 clampedPrev = curPixel.color;
    if (historyAccepted) {
        // 4) compute variance estimate from prev moments
        vec3 prevVar = max(prevM2 - prevM1 * prevM1, vec3(1e-6f)); // channel-wise variance floor
        // 5) optional: clamp the sampled prevM1 to current +- k * sigma
        vec3 sigma = sqrt(prevVar);
        vec3 minv = curPixel.color - clampK * sigma;
        vec3 maxv = curPixel.color + clampK * sigma;
        clampedPrev = clamp(prevM1, minv, maxv);
    }
    // 6) temporal blending (exponential moving average)
    vec3 newM1;
    vec3 newM2;
    float alpha = 1.0f / (1.0f + prevAge);
    alpha = clamp(alpha, 0.02f, 0.95f);
    float colorDiff = length(curPixel.color - clampedPrev);
    float normalDiff = 1.0f - dot(curPixel.normal, prevNormal);
    float similarity = exp(-colorDiff * 40.0f) * exp(-normalDiff * 10.0f);
    float adaptiveAlpha = clamp(alpha * similarity, 0.02f, 1.0f);

    if (historyAccepted) {
        // M1_new = lerp(clampedPrev, cur.color, alpha)
        newM1 = mix(clampedPrev, curPixel.color, adaptiveAlpha);
        // M2_new = lerp(prevM2, cur.color*cur.color, alpha)
        vec3 curSq = curPixel.color * curPixel.color;
        newM2 = mix(prevM2, curSq, adaptiveAlpha);
        tempBuffer[idx].age = prevAge + 1.0f;
    } else {
        newM1 = curPixel.color;
        newM2 = curPixel.color * curPixel.color;
        tempBuffer[idx].age = 1.0f;
    }

    tempBuffer[idx].M1 = newM1;
    tempBuffer[idx].M2 = newM2;
    return { newM1, 1.0f };
}

void AtrousDenoiser::Denoise(int x, int y, const PixelInfo* input, vec4* output)
{
    gBuffer = input;
    step = 1;
    for (int i = 0; i < width * height; i++) {
        // use temporal output or path trace output as input
        ping[i] = output[i];
    }

    auto& t = Task::GetInstance();
    for (int i = 0; i < iteration; i++) {
        t.Parallel2D(width, height, 32, [this, &output](uint16_t x, uint16_t y) {
            output[y * width + x] = AtrousDenoise(x, y);
        });
        sigmaColor = sigmaColor0 * powf(0.5f, i);
        step = 1 << i;
        swap(ping, pong);
    }
}

// Edge-Avoiding À-Trous Wavelet Transform for fast Global Illumination Filtering(2010)
vec4 AtrousDenoiser::AtrousDenoise(uint16_t x, uint16_t y)
{
    int p = y * width + x;
    const PixelInfo& center = gBuffer[p];
    vec3 centerColor = ping[p]; // must use new color

    vec3 sumColor = vec3(0);
    float sumWeight = 0.0f;
    for (int dy = -radius.x; dy <= radius.x; ++dy) {
        for (int dx = -radius.x; dx <= radius.x; ++dx) {
            int sx = x + dx * step;
            int sy = y + dy * step;
            if (sx < 0 || sy < 0 || sx >= width || sy >= height) { continue; }
            int q = sy * width + sx;
            const PixelInfo& nb = gBuffer[q]; // must use new color
            vec3 nbColor = ping[q];
            // color weight
            float wRt = 1.0f;
            if (step > 1) {
                float dc = glm::length(centerColor - nbColor);
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
    return { pong[p], 1.0 };
}
