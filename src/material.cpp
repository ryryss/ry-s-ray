#include "material.h"
#include "loader.h"
using namespace std;
using namespace ry;
void ry::Material::SetRawPtr(tinygltf::Model* pModel, tinygltf::Material* pMat)
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

Spectrum Material::GetAlbedo(const Interaction& isect) const
{
	return baseColorFactor * GetTexture(isect);
}

unique_ptr<BSDF> Material::CreateBSDF(const Interaction& si) const
{
    auto& pbr = m->pbrMetallicRoughness;
    auto baseColor = GetAlbedo(si);
    unique_ptr<BSDF> b = make_unique<BSDF>(si.normal);

    // diffuse
    if (pbr.metallicFactor < 1.0f) {
        Spectrum diffuseColor = baseColor * (1.0f - (float)pbr.metallicFactor);
        if (!diffuseColor.IsBlack()) {
            b->Add(make_unique<LambertianReflection>(diffuseColor));
        }
    }

    // specular GGX/Trowbridge-Reitz
    // Spectrum F0 = glm::mix(Spectrum(0.04f).c, baseColor.c, pbr.metallicFactor);
    auto distrib = make_shared<TrowbridgeReitzDistribution>(
        pbr.roughnessFactor, pbr.roughnessFactor);
    auto fresnel = make_shared<FresnelNoOp>();
    b->Add(make_unique<MicrofacetReflection>(
        baseColor, distrib, fresnel));
	return b;
}

vec4 ry::Material::GetTexture(const Interaction& si) const
{
    if (image == nullptr) {
        return vec4(1.0f);
    }
    const auto& bary = si.bary;
    const auto& vts = si.vts;
    vec2 uv = bary[0] * vts[0]->uv + bary[1] * vts[1]->uv + bary[2] * vts[2]->uv;
    uint32_t x = uv[0] * (image->width - 1);
    uint32_t y = uv[1] * (image->height - 1); // no need reverse
    // int((1.0f - v) * (image.height - 1));
    int idx = (y * image->width + x) * image->component;
    auto pixel = image->image.data();
    return { pixel[idx + 0] / 255.0f, pixel[idx + 1] / 255.0f,
             pixel[idx + 2] / 255.0f, (image->component == 4) ? pixel[idx + 3] / 255.0f : 1.0f };
}