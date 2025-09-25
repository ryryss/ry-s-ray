#pragma once
#include "bsdf.h"
namespace ry{
class Material {
public:
	Material() {}
	void SetRawPtr(tinygltf::Model* pModel, tinygltf::Material* pMat);
	inline bool IsEmissive() {
	    /*(m->emissiveFactor[0] > 0.0f ||
		 m->emissiveFactor[1] > 0.0f ||
		 m->emissiveFactor[2] > 0.0f);*/
		return emissiveStrength != 0.0;
	}
	inline float GetEmissiveStrength() { return emissiveStrength; }
    virtual vec4 GetAlbedo(const vec2& uv) const;
	virtual vec4 GetAlbedo() const { return baseColorFactor; };
private:
	vec4 ry::Material::GetTexture(const vec2& uv) const;

	tinygltf::Material* m = nullptr;
	tinygltf::Model* model = nullptr;
	// TODO texture and sample
	tinygltf::Image* image = nullptr;

	ry::vec4 baseColorFactor;
	float emissiveStrength = 0.0;
};
}