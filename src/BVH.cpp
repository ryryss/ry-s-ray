#include "loader.h"
using namespace std;
using namespace ry;

BVH::BVH(uint8_t geomCnt, int m) : maxLeafSize(m) {
    std::vector<uint64_t> indices(geomCnt);
    std::iota(indices.begin(), indices.end(), 0);
    root = BuildNode(indices);
}

AABB BVH::ComputeBounds(const vector<uint64_t>& indices)
{
    auto& tris = Loader::GetInstance().GetTriangles();
    AABB box;
    for (auto idx : indices) {
        const Triangle& t = tris[idx];
        box.expand(t.vts[0]->pos);
        box.expand(t.vts[1]->pos);
        box.expand(t.vts[2]->pos);
    }
    return box;
}

std::shared_ptr<BVHNode> BVH::BuildNode(vector<uint64_t>& indices)
{
    auto& tris = Loader::GetInstance().GetTriangles();
    auto node = std::make_shared<BVHNode>();
    node->box = ComputeBounds(indices);
    if (indices.size() <= maxLeafSize) {
        node->indices = indices; // store idx in leaf
        bug += indices.size();
        return node;
    }
    // chose a axis
    vec3 diag = node->box.max - node->box.min;
    int axis = 0;
    if (diag.y > diag.x) axis = 1;
    if (diag.z > diag[axis]) axis = 2;
    // use triangle centroid to sort
    std::sort(indices.begin(), indices.end(),
        [&](int a, int b) {
            vec3 ca = (tris[a].vts[0]->pos + tris[a].vts[1]->pos + tris[a].vts[2]->pos) / 3.0f;
            vec3 cb = (tris[b].vts[0]->pos + tris[b].vts[1]->pos + tris[b].vts[2]->pos) / 3.0f;
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
        node = root;
    }

    if (node->isLeaf()) {
        res.insert(res.end(), node->indices.begin(), node->indices.end());
        return;
    }

    if (!node->box.Hit(ray, 0.0f, std::numeric_limits<float>::infinity())) {
        return;
    }

    TraverseBVH(res, ray, node->l);
    TraverseBVH(res, ray, node->r);
}
