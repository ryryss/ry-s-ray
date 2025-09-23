#include "BVH.h"
#include "loader.h"
using namespace std;
using namespace ry;

BVH::BVH(uint64_t geomCnt, int m) : maxLeafSize(m) {
    std::vector<uint64_t> indices(geomCnt);
    std::iota(indices.begin(), indices.end(), 0);
    root = BuildNode(indices);
}

void BVH::ComputeBounds(BVHNode& b, const vector<uint64_t>& indices)
{
    auto& tris = Loader::GetInstance().GetTriangles();
    for (auto idx : indices) {
        auto vts = Loader::GetInstance().GetTriVts(idx);
        b.box.expand(vts[0]->pos);
        b.box.expand(vts[1]->pos);
        b.box.expand(vts[2]->pos);

        b.cBox.expand(tris[idx].c);
    }
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

    // chose a axis
    vec3 diag = node->cBox.max - node->cBox.min;
    uint64_t axis = 0;
    if (diag.y > diag.x) axis = 1;
    if (diag.z > diag[axis]) axis = 2;

    constexpr uint8_t bucketCount = 12;
    struct BucketInfo {
        int count = 0;
        AABB bounds;
    };
    std::vector<BucketInfo> buckets(bucketCount);

    auto& tris = Loader::GetInstance().GetTriangles();
    for (int idx : indices) {
        const Triangle& tri = tris[idx];
        vec3 c = tri.c;
        float relative = (c[axis] - node->cBox.min[axis]) /
            (node->cBox.max[axis] - node->cBox.min[axis] + 1e-6f);
        int b = std::clamp(int(relative * bucketCount), 0, bucketCount - 1);
        buckets[b].count++;
        AABB triBox;
        auto vts = Loader::GetInstance().GetTriVts(tri);
        triBox.expand(vts[0]->pos);
        triBox.expand(vts[1]->pos);
        triBox.expand(vts[2]->pos);
        buckets[b].bounds.expand(triBox);
    }
    // SAH 计算
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

    // 找到最优分裂位置
    float minCost = cost[0];
    int minSplit = 0;
    for (int i = 1; i < bucketCount - 1; i++) {
        if (cost[i] < minCost) {
            minCost = cost[i];
            minSplit = i;
        }
    }

    // 如果 SAH 不划算，就建叶子
    if (minCost >= indices.size()) {
        node->indices = indices;
        idxCnt += indices.size();
        PrintIdx(node);
        return node;
    }

    // 分裂 indices
    std::vector<uint64_t> leftIndices, rightIndices;
    for (int idx : indices) {
        const Triangle& tri = tris[idx];
        vec3 c = tri.c;
        float relative = (c[axis] - node->cBox.min[axis]) /
            (node->cBox.max[axis] - node->cBox.min[axis] + 1e-6f);
        int b = std::clamp(int(relative * bucketCount), 0, bucketCount - 1);
        if (b <= minSplit) leftIndices.push_back(idx);
        else rightIndices.push_back(idx);
    }
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
    if (!node->box.Intersect(ray, 0, floatMax)) {
        return;
    }
    TraverseBVH(res, ray, node->l);
    TraverseBVH(res, ray, node->r);
}
