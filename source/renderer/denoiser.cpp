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
    memccpy(prevGBuffer.data(), gBuffer, width * height, width * height);
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
    auto& t = Task::GetInstance();
    step = 1;
    for (int i = 0; i < iteration; i++) {
        t.Parallel2D(width, height, 32, [this, &output](uint16_t x, uint16_t y) {
            output[y * width + x] = AtrousDenoise(x, y);
        });
        sigmaColor = sigmaColor0 * powf(0.5f, i);
        step = 1 << i;
        swap(ping, pong);
    }
}

vec4 AtrousDenoiser::AtrousDenoise(uint16_t x, uint16_t y)
{
    int idx = y * width + x;
    const PixelInfo& center = gBuffer[idx];
    vec3 centerColor = ping[idx];

    vec3 sumColor = vec3(0);
    float sumWeight = 0.0f;

    const static float kernel[5] = { 1.f / 16, 1.f / 4, 3.f / 8, 1.f / 4, 1.f / 16 };

    for (int dy = -radius.x; dy <= radius.x; ++dy) {
        for (int dx = -radius.x; dx <= radius.x; ++dx) {
            int sx = x + dx * step;
            int sy = y + dy * step;
            if (sx < 0 || sy < 0 || sx >= width || sy >= height) { continue; }
            int sIdx = sy * width + sx;
            const PixelInfo& nb = gBuffer[sIdx];
            vec3 nbColor = ping[sIdx];

            float wRt = 1.0f;
            if (iteration > 0) {
                float dc = glm::length(centerColor - nbColor);
                wRt = expf(-(dc * dc) / (sigmaColor * sigmaColor));
            }

            float dn = glm::length(center.normal - nb.normal);
            float w_n = expf(-(dn * dn) / (sigmaNormal * sigmaNormal));

            float dxp = glm::length(center.position - nb.position);
            float w_x = expf(-(dxp * dxp) / (sigmaPosition * sigmaPosition));

            float w = wRt * w_n * w_x;

            float kernel_w = kernel[dx + radius.x] * kernel[dy + radius.x];
            sumColor += nb.color * w * kernel_w;
            sumWeight += w * kernel_w;
        }
    }
    pong[idx] = sumWeight > 1e-5f ? sumColor / sumWeight : center.color;
    return { pong[idx], 1.0 };
}
