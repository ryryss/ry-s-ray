#include "denoiser.h"
#include "sampler.hpp"
using namespace ry;
using namespace std;

vec3 BilateralDenoiser::Denoise(int x, int y)
{
    auto idx = [&](int x, int y) { return y * width + x; };
    const PixelInfo& c = gBuffer[idx(x, y)];

    vec3 sumColor(0.0f);
    float sumWeight = 0.0f;
    for (int dy = -radius.y; dy <= radius.y; dy++) {
        for (int dx = -radius.x; dx <= radius.x; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;

            const PixelInfo& neighbor = gBuffer[idx(nx, ny)];

            float spatialDist2 = float(dx * dx + dy * dy);
            float wSpatial = std::exp(-spatialDist2 / (2 * sigmaSpatial * sigmaSpatial));

            float colorDist2 = dot(c.color - neighbor.color, c.color - neighbor.color);
            float wColor = exp(-colorDist2 / (2 * sigmaColor * sigmaColor));

            float normalDist = std::max(0.0f, 1.0f - dot(c.normal, neighbor.normal));
            float wNormal = std::exp(-normalDist * normalDist / (2 * sigmaNormal * sigmaNormal));

            float depthDiff = fabs(c.depth - neighbor.depth);
            float wDepth = exp(-depthDiff * depthDiff / (2.0f * sigmaDepth * sigmaDepth));

            float wAlbedo = 1.0f;
            if (sigmaAlbedo > 0.0f) {
                float albedoDist2 = length(c.albedo - neighbor.albedo);
                wAlbedo = std::exp(-albedoDist2 * albedoDist2 / (2 * sigmaAlbedo * sigmaAlbedo));
            }

            float weight = wSpatial * wColor * wNormal * wAlbedo * wDepth;

            sumColor += neighbor.color * weight;
            sumWeight += weight;
        }
    }
    return sumWeight > 0.0f ? sumColor / sumWeight : c.color;
}

vec3 TemporalDenoiser::Denoise(int x, int y)
{
    int idx = y * width + x;
    const PixelInfo& curPixel = gBuffer[idx];
    // 1) compute previous pixel location via motion vector
    vec2 prevPixel = vec2(x, y) + curPixel.motion;
    vec2 prevUV = prevPixel / vec2(width, height);
    if (prevUV.x < 0.0 || prevUV.x > 1.0 || prevUV.y < 0.0 || prevUV.y > 1.0) {
        tempBuffer[idx].M1 = curPixel.color;
        tempBuffer[idx].M2 = curPixel.color * curPixel.color;
        tempBuffer[idx].age = 1.0f;
        return curPixel.color;
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
    if (std::abs(curPixel.depth - prevDepth) > depthThresh) { historyAccepted = false; }
    
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
    alpha = clamp(alpha, 0.02f, 0.95f);        // protect extremes

    vec3 diff = curPixel.color - clampedPrev;
    float colorDiff = length(diff);             // 颜色差
    float normalDiff = 1.0f - dot(curPixel.normal, prevNormal); // 法线角度差（需保存法线历史）
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
    return newM1;
}
