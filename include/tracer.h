#ifndef	RAY_TRACE
#define RAY_TRACE
#include "pub.h"
#include "loder.h"
class Tracer {
public:
	Tracer() {};
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
	std::pair<ry::vec3, ry::vec3> RayGeneration(uint32_t x, uint32_t y);
	bool RayCompute(uint32_t x, uint32_t y);
	void RayShading(uint16_t x, uint16_t y, ry::Triangle& t, const ry::vec3& hitPoint);
	bool ShadowShading(const ry::vec3& hitPoint, const ry::vec3& n, const ry::vec3& l, const float dis);
	ry::vec4 Tracer::SampleTexture(const ry::vec3& bary, const ry::vec2& uv0, 
		const ry::vec2& uv1, const ry::vec2& uv2);

	ry::Screen scr;
	std::vector<ry::vec4> pixels;

	/* 
	   Actually i do not want tracer.h include other head file except math
	   but keep a data struct ptr can easy to handle data.
	   To fully decouple, we could add an intermediate class to call loader and tracer.
	 */
	Loader* model;
};
#endif