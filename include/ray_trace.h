#ifndef	RAY_TRACE
#define RAY_TRACE
#include "pub.h"

class RayTrace {
public:
	RayTrace() {};
	void SetCamera(const ry::Camera& c);
	void SetLight(const ry::Light& l) {
		lgt = l;
	}
	std::vector<uint8_t > Excute(const ry::Screen& s);
	std::vector<uint8_t >& GetResult() {
		return colors;
	}
	void SetData(const std::vector<ry::Triangle>* t) {
		input = t;
	}
private:
	void RayGeneration();
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

	const std::vector<ry::Triangle>* input;
};
#endif