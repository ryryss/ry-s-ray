#pragma once
#include "node.hpp"
#include "interaction.hpp"
#include "sample.hpp"
namespace ry{
class Scene;

class Light : public Node {
public:
    Light() {};
    Light(const ry::Node& other) : ry::Node(other) {};
    Spectrum Sample_Li(const Scene* scene, const ry::Interaction* isect, ry::vec3& wi) const;

    float area = 0.0f;
    float emissiveStrength;
    Spectrum I; // or emissiveFactor
    std::vector<uint64_t> triangles;
};
}