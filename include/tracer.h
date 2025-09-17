#ifndef	RAY_TRACE
#define RAY_TRACE
#include "pub.h"
#include "loader.h"
#include "BSDF.h"

class Tracer {
public:
	Tracer();
	void Excute(const ry::Screen& s);
	std::vector<ry::vec4>& GetResult() {
		return pixels;
	}
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
	std::vector<ry::vec4> pixels;
	float tMin, tMax;
	uint16_t maxTraces = 16; // for ervery pixel
};
#endif