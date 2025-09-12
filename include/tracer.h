#ifndef	RAY_TRACE
#define RAY_TRACE
#include "pub.h"
#include "loder.h"
#include "BSDF.h"

struct Ray {
	ry::vec3 o;
	ry::vec3 d;
};

class Sampler {
public:
	Sampler(){}
	ry::vec2 Get2D() {
		return ry::vec2(glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.0f));
	}
	float Get1D() {
		return glm::linearRand(0.0f, 1.0f);
	}
};

struct Interaction { // now just triangle
	ry::Triangle tri;
	ry::vec3 p;
	float tMin;
	std::unique_ptr<BSDF> b;
	Interaction() {}
	Interaction(const ry::Triangle& t, float mint) : tri(t), tMin(mint) {}
};

class Tracer {
public:
	Tracer();
	void Excute(const ry::Screen& s);
	std::vector<ry::vec4>& GetResult() {
		return pixels;
	}
	inline void SetModel(Loader* m) {
		model = m;
	}
	void ProcessCamera();
private:
	void Parallel();
	Ray RayGeneration(uint32_t x, uint32_t y);
	Spectrum Tracer::RayCompute(uint32_t x, uint32_t y);
	// or named radiance(), L = Radiance, i = incoming
	Spectrum Tracer::Li(const Ray& r);
	ry::vec4 Tracer::SampleTexture(const ry::vec3& bary, const ry::vec2& uv0, 
		const ry::vec2& uv1, const ry::vec2& uv2);
	bool Intersect(const Ray& r, const float dis, Interaction* isect);
	bool IntersectAny(const Ray& r, const float dis, Interaction* isect); // hit any then stop immediately

	Spectrum EstimateDirect(const Interaction& isect);

	ry::Screen scr;
	std::vector<ry::vec4> pixels;
	float tMin, tMax;
	uint16_t maxTraces = 16; // for ervery pixel
	// BSDF* bxdf;
	/*
	   Actually i do not want tracer.h include other head file except math
	   but keep a data struct ptr can easy to handle data.
	   To fully decouple, we could add an intermediate class to call loader and tracer.
	*/
	Loader* model;
};
#endif