#pragma once
#include "node.hpp"
#include "interaction.hpp"
#include "sample.hpp"
namespace ry{
class Scene;

class Light : public Node {
public:
    Light() {};
    Light(const Node& other) : Node(other) {};
    Spectrum Sample_Li(const Scene* scene, const Interaction* isect, vec3* wi, float* pdf) const;
    uint16_t SamplePoint(const Scene* scene, vec3& pos, vec3& n) const;
    float area = 0.0f;
    float emissiveStrength;
    Spectrum I; // or emissiveFactor
    std::vector<uint64_t> triangles;
};
}