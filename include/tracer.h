#ifndef	RAY_TRACE
#define RAY_TRACE
#include "pub.h"
#include "loder.h"
class Tracer {
public:
	Tracer() {};
	std::vector<uint8_t > Excute(const ry::Screen& s);
	std::vector<uint8_t >& GetResult() {
		return colors;
	}
	inline void SetModel(Loader* m) {
		model = m;
	}
	void ProcessCamera();
private:
	void Calculate();
	bool RayCompute(uint32_t x, uint32_t y);
	void RayShading(uint16_t x, uint16_t y);

	ry::Camera cam;
	ry::Light lgt;
	ry::Screen scr;
	std::vector<uint8_t > colors;

	ry::vec3 up; // up
	ry::vec3 f; // forward
	ry::vec3 l; // location

	// some mid vec by base vec cal
	ry::vec3 u, v, w;

	/* 
	   Actually i do not want ray_trace.h include other head file except math
	   but keep a data struct ptr can easy to handle data.
	   To fully decouple, we could add an intermediate class to call loader and tracer.
	 */
	Loader* model;
};
#endif