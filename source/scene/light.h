#pragma once
#include "node.hpp"
#include "interaction.hpp"
#include "sample.hpp"
namespace ry{
class Model;

class Light : public Node {
public:
    Light(Model* m) : model(m) {};
    Light(const Node& other) : Node(other) {};
    Spectrum Sample_Li(const Interaction* isect, vec3* wi, float* pdf) const;
    float Pdf(const vec3& wo, const vec3& wi) const;
    uint16_t SamplePoint(vec3& pos, vec3& n) const;
    float area = 0.0f;
    float emissiveStrength;
    Spectrum I; // or emissiveFactor
    std::vector<uint64_t> triangles;

    Model* model;
};
}