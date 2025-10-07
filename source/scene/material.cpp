#include "material.h"
#include "interaction.hpp"
#include "model.h"
using namespace std;
using namespace ry;
void Material::SetRawPtr(Model* pModel, gltf::Material* pMat)
{
    model = pModel;
    raw = pMat;
    auto& pbr = raw->pbrMetallicRoughness;
    if (auto idx = pbr.baseColorTexture.index; idx >= 0) {
        texture = &model->GetRaw().textures[idx];
    }
    baseColorFactor = vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1],
        pbr.baseColorFactor[2], pbr.baseColorFactor[3]);
}

vec4 Material::GetAlbedo(const vec2& uv) const
{
    return texture ? baseColorFactor * GetTexture(uv) : baseColorFactor;
}

unique_ptr<BSDF> Material::CreateBSDF(const Interaction* isect) const
{
    auto& pbr = raw->pbrMetallicRoughness;
    const auto& a = model->GetVertex(isect->tri->vertIdx[0]);
    const auto& b = model->GetVertex(isect->tri->vertIdx[1]);
    const auto& c = model->GetVertex(isect->tri->vertIdx[2]);
    vec2 uv = isect->bary[0] * a->uv + isect->bary[1] * b->uv + isect->bary[2] * c->uv;
    auto baseColor = GetAlbedo(uv);
    unique_ptr<BSDF> bsdf = make_unique<BSDF>(isect->normal);

    // diffuse
    if (pbr.metallicFactor < 1.0f - FloatEpsilon) {
        Spectrum diffuseColor = baseColor * (1.0f - (float)pbr.metallicFactor);
        if (!diffuseColor.IsBlack()) {
            bsdf->Add(make_unique<LambertianReflection>(diffuseColor));
        }
    } else { 
        // GGX/Trowbridge-Reitz
        Spectrum F0 = glm::mix(Spectrum(0.04f).c, vec3(baseColor), pbr.metallicFactor);
        auto distrib = make_shared<TrowbridgeReitzDistribution>(pbr.roughnessFactor, pbr.roughnessFactor);
        auto fresnel = make_shared<FresnelSchlick>(F0);
        bsdf->Add(make_unique<MicrofacetReflection>(vec3(1.0), distrib, fresnel));
    }
    return bsdf;
}

vec4 Material::GetTexture(const vec2& uv) const
{
    if (texture == nullptr) {
        return vec4(1.0f);
    }

    const auto& image = model->GetImage(texture->source);
    const auto& sampler = model->GetRaw().samplers[texture->sampler];
    auto color = TextureSampler::SampleTexture(&image, &sampler, uv, 0);
    // return mix( color / 12.92, pow((color + 0.055) / 1.055, vec3(2.4)), step(0.04045, color) );
    return pow(color, vec4(GammaLinear));
}