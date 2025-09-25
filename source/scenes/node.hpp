#include "pch.h"
namespace ry{
struct Node {
    std::string name;
    mat4 m = mat4(1.0f); // trans mat
    std::vector<uint32_t> c; // children
    int i = -1;
    std::string type = "node";
};

struct Camera : public Node {
    float znear = 0.0;
    float zfar = 0.0;

    // perspective
    double aspectRatio = 0.0;  // min > 0
    double yfov = 0.0;         // required. min > 0
    // orthographic
    double xmag = 0.0;   // required. must not be zero.
    double ymag = 0.0;   // required. must not be zero.

    vec3 w; // forward
    vec3 e; // location of eye(cam)
    vec3 v; // cam base up
    vec3 u; // cam base right

    Camera() {};
    Camera(const Node& other) : Node(other) {};
};
}