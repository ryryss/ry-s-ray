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
    Spectrum Sample_Li(const Scene* scene, const Interaction* isect, vec3& wi) const;
    vec3 SamplePoint(const Scene* scene) const;
    float area = 0.0f;
    float emissiveStrength;
    Spectrum I; // or emissiveFactor
    std::vector<uint64_t> triangles;
};
}