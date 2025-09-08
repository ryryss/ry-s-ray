#ifndef	RAYTRACE
#define RAYTRACE
#include "pub.h"
#include "loder.h"
#include "BxDF.h"
#include "light.h"

class Sampler {
public:
	std::mt19937 rng;
	std::uniform_real_distribution<float> dist;

	Sampler(unsigned seed = 42) : rng(seed), dist(0.0f, 1.0f) {}

	ry::vec2 Get2D() {
		return ry::vec2(dist(rng), dist(rng));
	}

	float Get1D() {
		return dist(rng);
	}
};

struct Ray {
	ry::vec3 o;
	ry::vec3 d;
};

struct Interaction { // now just triangle
	ry::Triangle t;
	ry::vec3 p; // hit
	float mt;
	Interaction() {}
	Interaction(const ry::Triangle& tri, float mint) : t(tri), mt(mint) {}
};

class Tracer {
public:
	Tracer() { bxdf = new BSDF(BSDF_ALL); };
	void Excute(const ry::Screen& s);
	std::vector<ry::vec4>& GetResult() {
		return pixels;
	}
	inline void SetModel(Loader* m) {
		model = m;
	}
	void ProcessCamera();
private:
	void Calculate();
	Ray RayGeneration(uint32_t x, uint32_t y);
	bool Intersect(const Ray& r, const float dis, Interaction* isect);
	ry::vec4 Tracer::SampleTexture(const ry::vec3& bary, const ry::vec2& uv0, 
		const ry::vec2& uv1, const ry::vec2& uv2);

	// or named radiance(), L = Radiance, i = incoming
	Spectrum Li(const Ray& r, int depth);
	Spectrum EstimateDirect(Interaction& isect);
	ry::Screen scr;
	std::vector<ry::vec4> pixels;

	float tMin, tMax;
	BSDF* bxdf;
	/* 
	   Actually i do not want tracer.h include other head file except math
	   but keep a data struct ptr can easy to handle data.
	   To fully decouple, we could add an intermediate class to call loader and tracer.
	 */
	Loader* model;

	static uint8_t maxDeep;
};
#endif