#include "bvh.h"
#include "scene.h"
using namespace std;
using namespace ry;

BVH::BVH(Scene* s, uint64_t geomCnt, int m) : scene(s), maxLeafSize(m) {
    std::vector<uint64_t> indices(geomCnt);
    std::iota(indices.begin(), indices.end(), 0);
    root = BuildNode(indices);
}

void BVH::ComputeBounds(BVHNode& node, const vector<uint64_t>& indices)
{
    for (int i = 0; i < indices.size(); i++) {
        auto& tri = scene->triangles[indices[i]];
        const auto& a = scene->GetVertex(tri.vertIdx[0])->pos;
        const auto& b = scene->GetVertex(tri.vertIdx[1])->pos;
        const auto& c = scene->GetVertex(tri.vertIdx[2])->pos;
        node.box.expand(a);
        node.box.expand(b);
        node.box.expand(c);

        node.cBox.expand(tri.c);
    }
    /*expand diagonally by a certain percentage
    ry::vec3 delta = (b.box.max - b.box.min) * (0.5f);
    b.box.min -= delta;
    b.box.max += delta;*/
}

std::shared_ptr<BVHNode> BVH::BuildNode(vector<uint64_t>& indices)
{
    auto node = std::make_shared<BVHNode>();
    node->i = leafCnt++;
    ComputeBounds(*node.get(), indices);

    if (indices.size() <= maxLeafSize) {
        node->indices = indices; // store idx in leaf
        PrintIdx(node);
        return node;
    }

    vector<uint64_t> left;
    vector<uint64_t> right;
    if (!SAHSplit(node, indices, left, right)) {
        node->indices = indices;
        PrintIdx(node);
        return node;
    }
    node->l = BuildNode(left);
    node->r = BuildNode(right);
    return node;
}

bool BVH::SAHSplit(const shared_ptr<BVHNode> node, const vector<uint64_t>& indices, vector<uint64_t>& l, vector<uint64_t>& r)
{
    auto axis = node->SplitAxis();
    constexpr uint8_t bucketCount = 12;
    struct BucketInfo {
        int count = 0;
        AABB bounds;
    };
    vector<BucketInfo> buckets(bucketCount);

    for (int i = 0; i < indices.size(); i++) {
        auto& tri = scene->triangles[indices[i]];
        float relative = (tri.c[axis] - node->cBox.min[axis]) /
            (node->cBox.max[axis] - node->cBox.min[axis] + 1e-6f);
        int count = clamp(int(relative * bucketCount), 0, bucketCount - 1);
        buckets[count].count++;
        AABB box;
        const auto& a = scene->GetVertex(tri.vertIdx[0])->pos;
        const auto& b = scene->GetVertex(tri.vertIdx[1])->pos;
        const auto& c = scene->GetVertex(tri.vertIdx[2])->pos;
        box.expand(a);
        box.expand(b);
        box.expand(c);
        buckets[count].bounds.expand(box);
    }
    // cal SAH
    float cost[bucketCount - 1];
    for (int i = 0; i < bucketCount - 1; i++) {
        AABB b0, b1;
        int c0 = 0, c1 = 0;

        for (int j = 0; j <= i; j++) {
            if (buckets[j].count > 0) {
                b0.expand(buckets[j].bounds);
                c0 += buckets[j].count;
            }
        }
        for (int j = i + 1; j < bucketCount; j++) {
            if (buckets[j].count > 0) {
                b1.expand(buckets[j].bounds);
                c1 += buckets[j].count;
            }
        }
        cost[i] = 1.0f + (c0 * b0.surfaceArea() + c1 * b1.surfaceArea()) / node->box.surfaceArea();
    }
    // find best split
    float minCost = cost[0];
    int minSplit = 0;
    for (int i = 1; i < bucketCount - 1; i++) {
        if (cost[i] < minCost) {
            minCost = cost[i];
            minSplit = i;
        }
    }
    // can not split
    if (minCost >= indices.size()) {
        return false;
    }
    // split idx
    for (int i = 0; i < indices.size(); i++) {
        auto& tri = scene->triangles[indices[i]];
        float relative = (tri.c[axis] - node->cBox.min[axis]) /
            (node->cBox.max[axis] - node->cBox.min[axis] + 1e-6f);
        int b = clamp(int(relative * bucketCount), 0, bucketCount - 1);
        b <= minSplit ? l.push_back(indices[i]) : r.push_back(indices[i]);
    }
    return true;
}

void BVH::MidSplit(const shared_ptr<BVHNode> node, vector<uint64_t>& indices, vector<uint64_t>& l, vector<uint64_t>& r)
{
    // chose a axis
    auto axis = node->SplitAxis();
    // use triangle centroid to sort
    auto& tris = scene->triangles;
    std::sort(indices.begin(), indices.end(),
        [&](uint64_t a, uint64_t b) {
            return tris[a].c[axis] < tris[b].c[axis];
        });
    // median-split
    size_t mid = indices.size() / 2;
    l = std::vector<uint64_t>(indices.begin(), indices.begin() + mid);
    r = std::vector<uint64_t>(indices.begin() + mid, indices.end());
}

void BVH::TraverseBVH(vector<uint64_t>& res, const Ray& ray, shared_ptr<BVHNode> node)
{
    if (!node) {
        return;
    }
    if (!node->box.Intersect(ray, -floatMax, floatMax)) {
        return;
    }
    if (node->isLeaf()) {
        res.insert(res.end(), node->indices.begin(), node->indices.end());
        return;
    }
    TraverseBVH(res, ray, node->l);
    TraverseBVH(res, ray, node->r);
}
