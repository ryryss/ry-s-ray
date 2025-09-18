#ifndef	BVH_H
#define BVH_H
#include "pub.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <limits>
#include <cstdint>
// AABB
struct AABB {
    AABB() {
        float inf = std::numeric_limits<float>::infinity();
        min = ry::vec3(inf, inf, inf);
        max = ry::vec3(-inf, -inf, -inf);
    }
    AABB(const ry::vec3& a, const ry::vec3& b) { min = a; max = b; }

    void expand(const ry::vec3& p) {
        min.x = std::min(min.x, p.x); min.y = std::min(min.y, p.y); min.z = std::min(min.z, p.z);
        max.x = std::max(max.x, p.x); max.y = std::max(max.y, p.y); max.z = std::max(max.z, p.z);
        // constexpr float eps = 1e-;
        // min -= ry::vec3(eps);
        // max += ry::vec3(eps);

    }
    void expand(const AABB& b) {
        expand(b.min); expand(b.max);
    }

    // Andrew Kensler
    bool Hit(const ry::Ray& r, double t_min, double t_max) const {
        for (int a = 0; a < 3; a++) {
            auto dInv = r.dInv[a];
            auto t0 = (min[a] - r.o[a]) * dInv;
            auto t1 = (max[a] - r.o[a]) * dInv;
            if (dInv < 0.0f) {
                std::swap(t0, t1);
            }
            t_min = t0 > t_min ? t0 : t_min;
            t_max = t1 < t_max ? t1 : t_max;
            if (t_max <= t_min){
                return false;
            }
        }
        return true;
    }
    ry::vec3 min;
    ry::vec3 max;
};

struct BVHNode {
    AABB box;                       
    std::shared_ptr<BVHNode> l;
    std::shared_ptr<BVHNode> r;
    std::vector<uint64_t> indices; // triangle Indices

    bool isLeaf() const { return l == nullptr && r == nullptr; }
};

class BVH {
public:
    // triangles should outlive BVH (we store indices)
    BVH(uint8_t geomCnt, int m = 4);

    AABB ComputeBounds(const std::vector<uint64_t>& indices);
    std::shared_ptr<BVHNode> BuildNode(std::vector<uint64_t>& indices);
    void TraverseBVH(std::vector<uint64_t>& res, const ry::Ray& ray, const std::shared_ptr<BVHNode> node = nullptr);
private:
    uint8_t maxLeafSize;
    std::shared_ptr<BVHNode> root;

    int bug = 0;
};
#endif