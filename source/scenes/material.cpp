#include "material.h"
#include "interaction.hpp"
#include "scene.h"
using namespace std;
using namespace ry;
void Material::SetRawPtr(gltf::Model* pModel, gltf::Material* pMat)
{
    model = pModel;
    m = pMat;
    auto& pbr = m->pbrMetallicRoughness;
    if (auto idx = pbr.baseColorTexture.index; idx >= 0) {
        image = &model->images[idx];
    }
    baseColorFactor = vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1],
        pbr.baseColorFactor[2], pbr.baseColorFactor[3]);
}

vec4 Material::GetAlbedo(const vec2& uv) const
{
    return baseColorFactor * GetTexture(uv);
}

unique_ptr<BSDF> Material::CreateBSDF(const Scene* s, const Interaction* isect) const
{
    auto& pbr = m->pbrMetallicRoughness;
    const auto& a = s->GetVertex(isect->tri->vertIdx[0]);
    const auto& b = s->GetVertex(isect->tri->vertIdx[1]);
    const auto& c = s->GetVertex(isect->tri->vertIdx[2]);
    vec2 uv = isect->bary[0] * a->uv + isect->bary[1] * b->uv + isect->bary[2] * c->uv;
    auto baseColor = GetAlbedo(uv);
    unique_ptr<BSDF> bsdf = make_unique<BSDF>(isect->tri->normal);

    // diffuse
    if (pbr.metallicFactor < 1.0f - FloatEpsilon) {
        Spectrum diffuseColor = baseColor * (1.0f - (float)pbr.metallicFactor);
        if (!diffuseColor.IsBlack()) {
            bsdf->Add(make_unique<LambertianReflection>(diffuseColor));
        }
    } else {
        // specular GGX/Trowbridge-Reitz
        // Spectrum F0 = glm::mix(Spectrum(0.04f).c, baseColor.c, pbr.metallicFactor);
        auto distrib = make_shared<TrowbridgeReitzDistribution>(pbr.roughnessFactor, pbr.roughnessFactor);
        auto fresnel = make_shared<FresnelNoOp>();
        bsdf->Add(make_unique<MicrofacetReflection>(vec3(baseColor), distrib, fresnel));
    }
    return bsdf;
}

vec4 Material::GetTexture(const vec2& uv) const
{
    if (image == nullptr) {
        return vec4(1.0f);
    }
    uint32_t x = uv[0] * (image->width - 1);
    uint32_t y = uv[1] * (image->height - 1); // no need reverse
    // int((1.0f - v) * (image.height - 1));
    int idx = (y * image->width + x) * image->component;
    auto pixel = image->image.data();
    vec3 color = { pixel[idx + 0] / 255.0f,
                   pixel[idx + 1] / 255.0f,
                   pixel[idx + 2] / 255.0f};
    // gamma
    color = pow(color, vec3(2.2));
    return { color, (image->component == 4) ? pixel[idx + 3] / 255.0f : 1.0f };
}