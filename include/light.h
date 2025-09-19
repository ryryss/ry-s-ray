#ifndef LIGHT_H
#define LIGHT_H
#include "pub.h"
#include "BSDF.h"

namespace ry{
class Light : public ry::Node {
public:
    Light() {};
    Light(const ry::Node& other) : ry::Node(other) {};
    Spectrum Sample_Li(const ry::Sampler& s, const ry::Interaction* isect, ry::vec3& wi);

    float area = 0.0f;
    float emissiveStrength;
    Spectrum I; // or emissiveFactor
    std::vector<uint64_t> tris;
};
}
#endif