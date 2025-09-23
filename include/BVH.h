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
    }
    void expand(const AABB& b) {
        expand(b.min); expand(b.max);
    }
    float surfaceArea() const {
        ry::vec3 d = max - min;
        return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }
    bool Intersect(const ry::Ray& r, float tmin, float tmax) const {
        for (int i = 0; i < 3; i++) {
            float dInv = r.dInv[i];
            float t0 = (min[i] - r.o[i]) * dInv;
            float t1 = (max[i] - r.o[i]) * dInv;
            if (dInv < 0.0f) {
                std::swap(t0, t1);
            }
            tmin = std::fmax(t0 ,tmin); // enter
            tmax = std::fmin(t1, tmax); // leave
            if (tmax <= tmin) {
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
    AABB cBox; // centroid;
    std::shared_ptr<BVHNode> l;
    std::shared_ptr<BVHNode> r;
    std::vector<uint64_t> indices; // triangle Indices
    uint64_t i;
    bool isLeaf() const { return l == nullptr && r == nullptr; }
    uint8_t SplitAxis() {
        // chose a axis
        ry::vec3 diag = cBox.max - cBox.min;
        uint8_t axis = 0;
        if (diag.y > diag.x) { axis = 1; }
        if (diag.z > diag[axis]) { axis = 2; }
        return axis;
    }
};

class BVH {
public:
    // triangles should outlive BVH (we store indices)
    BVH(uint64_t geomCnt, int m = 4);

    void ComputeBounds(BVHNode& b, const std::vector<uint64_t>& indices);
    std::shared_ptr<BVHNode> BuildNode(std::vector<uint64_t>& indices);
    bool BVH::SAHSplit(const std::shared_ptr<BVHNode> node, const std::vector<uint64_t>& indices,
        std::vector<uint64_t>& l, std::vector<uint64_t>& r);
    void BVH::MidSplit(const std::shared_ptr<BVHNode> node, std::vector<uint64_t>& indices,
        std::vector<uint64_t>& l, std::vector<uint64_t>& r);
    void TraverseBVH(std::vector<uint64_t>& res, const ry::Ray& ray, const std::shared_ptr<BVHNode> node = nullptr);
    std::shared_ptr<BVHNode> root;
private:
    void PrintIdx(std::shared_ptr<BVHNode> node) {
#ifdef DEBUG
        std::cout << "build leaf tri indices : ";
        for (auto& i : node->indices) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
#endif // DEBUG
    }
    uint8_t maxLeafSize;
    uint64_t leafCnt = 0;
};
#endif