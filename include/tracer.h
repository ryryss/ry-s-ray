#ifndef	RAY_TRACE
#define RAY_TRACE
#include "pub.h"
#include "loader.h"
#include "BSDF.h"

class Tracer {
public:
	Tracer();
	void Excute();
	void SetInOutPut(const ry::Screen& s, Loader* m, ry::vec4* p);
private:
	void Parallel();
	ry::Ray RayGeneration(uint32_t x, uint32_t y);
	Spectrum Tracer::RayCompute(uint32_t x, uint32_t y);
	// or named radiance(), L = Radiance, i = incoming
	Spectrum Tracer::Li(const ry::Ray& r);
	ry::vec4 Tracer::SampleTexture(const ry::vec3& bary, const ry::vec2& uv0, 
		const ry::vec2& uv1, const ry::vec2& uv2);

	Spectrum EstimateDirect(const ry::Interaction& isect);

	ry::Screen scr;
	ry::vec4* pixels;
	std::vector<ry::vec4> sppBuffer;
	float tMin, tMax;
	uint16_t maxTraces = 16; // spp
	uint16_t currentTraces = 0;
	Loader* model; // or scene
};
#endif