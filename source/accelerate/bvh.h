#pragma once
#include "public.h"
#include "math.h"
namespace ry {
struct BVHNode {
    vec3 bboxMin, bboxMax;
    int left, right;
    int start;
    int count;
    std::vector<uint32_t> tris; // just keep idxs
};
}