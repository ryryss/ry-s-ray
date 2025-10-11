#pragma once
#include "pch.h"
namespace ry {
struct MipMap {
    int width;
    int height;
    std::vector<float> pixels; // RGBA

    inline vec4 GetPixels(int x, int y) const {
        int idx = (y * width + x) * 4;
        const float* p = &pixels[idx];
        return vec4(p[0], p[1], p[2], p[3]);
    }
};

struct Image {
    std::vector <MipMap> mm;
};

/*enum TextureSamplerType {
    NEAREST = 0,
    LINEAR,
    LINEAR_MIPMAP_NEAREST,
    LINEAR_MIPMAP_LINEAR
};*/

namespace TextureSampler {
    /*
    #define TINYGLTF_TEXTURE_WRAP_REPEAT (10497)
    #define TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE (33071)
    #define TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT (33648)
    */
    inline float wrapCoord(float u, int mode)
    {
        switch (mode) {
        case 10497: // REPEAT
            return u - floor(u); // fract(u)
        case 33648: { // MIRRORED_REPEAT
            float frac = u - floor(u);
            int period = (int)floor(u);
            return (period % 2 == 0) ? frac : 1.0f - frac;
        }
        case 33071: // CLAMP_TO_EDGE
        default:
            return std::clamp(u, 0.0f, 1.0f);
        }
    }

    static vec4 NearestSample(const MipMap& mm, float u, float v)
    {
        return SampleNearest<vec4>(u, v, mm.width, mm.height,
            [&](int x, int y) { return mm.GetPixels(x, y);});
    }

    static vec4 LinearSample(const MipMap& mm, float u, float v)
    {
        return SampleBilinear<vec4>(u, v, mm.width, mm.height,
            [&](int x, int y) { return mm.GetPixels(x, y);});
    }
    /*
    #define TINYGLTF_TEXTURE_FILTER_NEAREST (9728)
    #define TINYGLTF_TEXTURE_FILTER_LINEAR (9729)
    #define TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST (9984)
    #define TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST (9985)
    #define TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR (9986)
    #define TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR (9987)
    */
    static vec4 SampleTexture(const Image* image, const gltf::Sampler* sampler, const vec2& uv, int lod)
    {
        float x = wrapCoord(uv.x, sampler->wrapS);
        float y = wrapCoord(uv.y, sampler->wrapT);

        int filter;
        const MipMap* mm;
        lod <= 0 ? filter = sampler->magFilter : filter = sampler->minFilter;

        int level0 = std::clamp((int)lod, 0, (int)image->mm.size() - 1);
        int level1 = std::min(level0 + 1, (int)image->mm.size() - 1);
        float lodFrac = lod - floor(lod);

        switch (filter) {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            return NearestSample(image->mm[0], x, y);

        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            return LinearSample(image->mm[0], x, y);

        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            return NearestSample(image->mm[level0], x, y);

        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            return LinearSample(image->mm[level0], x, y);

        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR: {
            vec4 c0 = NearestSample(image->mm[level0], x, y);
            vec4 c1 = NearestSample(image->mm[level1], x, y);
            return mix(c0, c1, lodFrac);
        }
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR: {
            vec4 c0 = LinearSample(image->mm[level0], x, y);
            vec4 c1 = LinearSample(image->mm[level1], x, y);
            return mix(c0, c1, lodFrac);
        }
        default:
            return NearestSample(image->mm[0], x, y);
        }
    }
}
}