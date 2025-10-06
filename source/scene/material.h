#pragma once
#include "bsdf.h"
namespace ry{
class Interaction;
class Model;
class Material {
public:
    Material() {}
    void SetRawPtr(Model* pModel, gltf::Material* pMat);
    inline void SetEmissiveInfo(const vec4& factor, float strength) {
        baseColorFactor = factor;
        emissiveStrength = strength;
    }
    inline bool IsEmissive() const {
        /*(m->emissiveFactor[0] > 0.0f ||
         m->emissiveFactor[1] > 0.0f ||
         m->emissiveFactor[2] > 0.0f);*/
        return emissiveStrength != 0.0;
    }
    inline bool IsSpecular() const {
        auto& pbr = raw->pbrMetallicRoughness;
        return pbr.roughnessFactor < 1e-6;
    }
    inline float GetEmissiveStrength() const { return emissiveStrength; }
    virtual vec4 GetAlbedo(const vec2& uv) const;
    virtual vec4 GetAlbedo() const { return baseColorFactor; };
    virtual vec4 GetEmission() const { return baseColorFactor * emissiveStrength; };
    std::unique_ptr<BSDF> CreateBSDF(const Interaction* isect) const;
private:
    vec4 GetTexture(const vec2& uv) const;

    gltf::Material* raw = nullptr;
    Model* model = nullptr;
    const gltf::Texture* texture = nullptr;

    vec4 baseColorFactor;
    float emissiveStrength = 0.0;
};
}