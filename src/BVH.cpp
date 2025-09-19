#include "loader.h"
using namespace std;
using namespace ry;

BVH::BVH(uint64_t geomCnt, int m) : maxLeafSize(m) {
    std::vector<uint64_t> indices(geomCnt);
    std::iota(indices.begin(), indices.end(), 0);
    root = BuildNode(indices);
}

AABB BVH::ComputeBounds(const vector<uint64_t>& indices)
{
    auto& tris = Loader::GetInstance().GetTriangles();
    AABB box;
    for (auto idx : indices) {
        auto vts = Loader::GetInstance().GetTriVts(idx);
        box.expand(vts[0]->pos);
        box.expand(vts[1]->pos);
        box.expand(vts[2]->pos);
    }
    return box;
}

std::shared_ptr<BVHNode> BVH::BuildNode(vector<uint64_t>& indices)
{
    auto node = std::make_shared<BVHNode>();
    node->box = ComputeBounds(indices);
    if (indices.size() <= maxLeafSize) {
        node->indices = indices; // store idx in leaf
        bug += indices.size();
        return node;
    }
    // chose a axis
    vec3 diag = node->box.max - node->box.min;
    uint64_t axis = 0;
    if (diag.y > diag.x) axis = 1;
    if (diag.z > diag[axis]) axis = 2;
    // use triangle centroid to sort
    std::sort(indices.begin(), indices.end(),
        [&](uint64_t a, uint64_t b) {
            auto vtsa = Loader::GetInstance().GetTriVts(a);
            auto vtsb = Loader::GetInstance().GetTriVts(b);
            vec3 ca = (vtsa[0]->pos + vtsa[1]->pos + vtsa[2]->pos) / 3.0f;
            vec3 cb = (vtsb[0]->pos + vtsb[1]->pos + vtsb[2]->pos) / 3.0f;
            return ca[axis] < cb[axis];
        });
    // median-split
    size_t mid = indices.size() / 2;
    std::vector<uint64_t> leftIndices(indices.begin(), indices.begin() + mid);
    std::vector<uint64_t> rightIndices(indices.begin() + mid, indices.end());

    node->l = BuildNode(leftIndices);
    node->r = BuildNode(rightIndices);
    return node;
}

void BVH::TraverseBVH(vector<uint64_t>& res, const Ray& ray, shared_ptr<BVHNode> node)
{
    if (!node) {
        return;
    }

    if (node->isLeaf()) {
        res.insert(res.end(), node->indices.begin(), node->indices.end());
        return;
    }


    if (!node->box.Hit(ray, 0, floatMax)) {
        return;
    }

    TraverseBVH(res, ray, node->l);
    TraverseBVH(res, ray, node->r);
}
