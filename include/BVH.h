#ifndef	BVH_H
#define BVH_H
#include "pub.h"
// AABB
struct AABB {
    AABB() {
        min = ry::vec3(floatMax);
        max = ry::vec3(-floatMax);
    }
    AABB(const ry::vec3& a, const ry::vec3& b) { min = a; max = b; }

    void expand(const ry::vec3& p) {
        min.x = std::min(min.x, p.x); min.y = std::min(min.y, p.y); min.z = std::min(min.z, p.z);
        max.x = std::max(max.x, p.x); max.y = std::max(max.y, p.y); max.z = std::max(max.z, p.z);

        // expand diagonally by a certain percentage
        ry::vec3 delta = (max - min) * (0.01f);
        min -= delta;
        max += delta;
    }
    void expand(const AABB& b) {
        expand(b.min); expand(b.max);
    }

    bool Intersect(const ry::Ray& r, float tmin, float tmax) const {
        for (int a = 0; a < 3; a++) {
            for (int i = 0; i < 3; i++) {
                float dInv = r.dInv[i];
                float t0 = (min[i] - r.o[i]) * dInv;
                float t1 = (max[i] - r.o[i]) * dInv;
                if (dInv < 0.0f) {
                    std::swap(t0, t1);
                }
                tmin = std::fmax(t0 ,tmin);
                tmax = std::fmin(t1, tmax);
                if (tmax <= tmin) {
                    return false;
                }
            }
            return true;
        }
    }
    ry::vec3 min;
    ry::vec3 max;
};

struct BVHNode {
    AABB box;                       
    std::shared_ptr<BVHNode> l;
    std::shared_ptr<BVHNode> r;
    std::vector<uint64_t> indices; // triangle Indices
    uint64_t i;
    bool isLeaf() const { return l == nullptr && r == nullptr; }
};

class BVH {
public:
    // triangles should outlive BVH (we store indices)
    BVH(uint64_t geomCnt, int m = 4);

    AABB ComputeBounds(const std::vector<uint64_t>& indices);
    std::shared_ptr<BVHNode> BuildNode(std::vector<uint64_t>& indices);
    void TraverseBVH(std::vector<uint64_t>& res, const ry::Ray& ray, const std::shared_ptr<BVHNode> node = nullptr);
    std::shared_ptr<BVHNode> root;
private:
    uint8_t maxLeafSize;
    uint64_t leafCnt = 0;
};
#endif