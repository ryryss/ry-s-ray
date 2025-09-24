#ifndef	MATERIAL_H
#define MATERIAL_H
#include "pub.h"
#include "BSDF.h"
namespace ry{
class Material {
public:
	Material() {}
	void SetRawPtr(tinygltf::Model* pModel, tinygltf::Material* pMat);
    virtual Spectrum GetAlbedo(const Interaction& si) const;
    virtual std::unique_ptr<BSDF> CreateBSDF(const Interaction& si) const;
private:
	vec4 GetTexture(const Interaction& si) const;

	tinygltf::Material* m = nullptr;
	tinygltf::Model* model = nullptr;
	ry::vec4 baseColorFactor;
	// TODO texture and sample
	tinygltf::Image* image = nullptr;
};
}
#endif