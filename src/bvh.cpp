#include "bvh.h"
#include <iostream>

//https://www.youtube.com/watch?v=C1H4zIiCOaI

namespace {
    bool intersectBB(const math::Ray& ray, const BBox& box, float tMin, float tMax, float& tHit)
    {
        for (int i = 0; i < 3; i++)
        {
            const float origin = ray.origin[i];
            const float dir = ray.direction[i];
            const float minB = box.min()[i];
            const float maxB = box.max()[i];
                        
            if (fabs(dir) < 1e-8f)
            {
                if (origin < minB || origin > maxB)
                    return false;
                continue;
            }

            const float invD = 1.0f / dir;
            float t0 = (minB - origin) * invD;
            float t1 = (maxB - origin) * invD;

            if (t0 > t1) std::swap(t0, t1);

            tMin = std::max(tMin, t0);
            tMax = std::min(tMax, t1);

            if (tMax < tMin)
                return false;
        }

        tHit = tMin;
        return true;
    }

}

BBox::BBox()
{    
    float inf = std::numeric_limits<float>::max();
    min_ = Vector3(inf, inf, inf);
    max_ = Vector3(-inf, -inf, -inf);
}

Vector3 BBox::center() const
{
	return (min_ + max_) * 0.5f;
}

Vector3 BBox::size() const
{
	return max_ - min_;
}

void BBox::growTo(const Vector3& point)
{
	min_ = ::min(min_, point);
	max_ = ::max(max_, point);
}

void BBox::growTo(const math::Triangle& t)
{
	growTo(t.a);
	growTo(t.b);
	growTo(t.c);
}


void BVH::build(const std::vector<math::Triangle>& triangles)
{	
	root_ = std::make_unique<Node>();

	for (const auto& t : triangles)
	{
		root_->box.growTo(t);
	}

	root_->triangles = triangles;

	split(*root_.get());
}

void BVH::print() const
{
    if (!root_)
    {
        std::cout << "BVH is empty.\n";
        return;
    }
    
    int emptyNodes = 0;
    int heavyNodes = 0;
    
    printNode(*root_, 0, emptyNodes, heavyNodes);
    
    std::cout << "--------------------------------\n";
    std::cout << "BVH Statistics:\n";
    std::cout << "Total Empty Nodes (0 tris): " << emptyNodes << "\n";
    std::cout << "Total Heavy Nodes (>8 tris): " << heavyNodes << "\n";
    std::cout << "--------------------------------\n";
}

void BVH::split(Node& parent, int depth) const
{
    if (depth > 10) return;
    if (parent.triangles.size() <= 2) return;

    Vector3 size = parent.box.size();

    int splitAxis =
        size.x() > std::max(size.y(), size.z()) ? 0 :
        size.y() > size.z() ? 1 : 2;


    float splitPos = 0.0f;
    for (const auto& tr : parent.triangles)
        splitPos += math::triangleCenter(tr)[splitAxis];
    splitPos /= parent.triangles.size();

    auto childA = std::make_unique<Node>();
    auto childB = std::make_unique<Node>();

    for (const auto& tr : parent.triangles)
    {
        float c = math::triangleCenter(tr)[splitAxis];
        Node* dst = (c < splitPos) ? childA.get() : childB.get();

        dst->triangles.push_back(tr);
        dst->box.growTo(tr);
    }

    if (childA->triangles.empty() || childB->triangles.empty())
    {
        return;
    }
    
    parent.childA = std::move(childA);
    parent.childB = std::move(childB);

    parent.triangles = {};
        
    split(*parent.childA, depth + 1);
    split(*parent.childB, depth + 1);
}

float BVH::intersect(const math::Ray& ray, float tMin, float tMax, math::Triangle& tr) const 
{
    return intersect(ray, *root_.get(), tMin, tMax, tr);
}

float BVH::intersect(const math::Ray& ray, const Node& node, float tMin, float tMax, math::Triangle& tr) const
{
    float tBox;    
    if (!intersectBB(ray, node.box, tMin, tMax, tBox))
        return tMax;
        
    if (node.triangles.empty())
    {
        math::Triangle trA, trB;

        float tHitA = intersect(ray, *node.childA, tMin, tMax, trA);
        
        float searchMaxB = std::min(tMax, tHitA);
        float tHitB = intersect(ray, *node.childB, tMin, searchMaxB, trB);

        if (tHitB < tHitA) {
            tr = trB;
            return tHitB;
        }
        else {
            tr = trA;
            return tHitA;
        }
    }
    else
    {
        float closestT = tMax;
        for (const auto& triangle : node.triangles)
        {
            float t = math::intersectTriangle(ray, triangle.a, triangle.b, triangle.c, tMin, closestT);
            if (t < closestT)
            {
                closestT = t;
                tr = triangle;
            }
        }
        return closestT;
    }
}


void BVH::printNode(const Node & node, int depth, int& emptyCount, int& heavyCount) const
{
    
    if (node.triangles.empty()) {
        emptyCount++;
    }
    if (node.triangles.size() > 8) {
        heavyCount++;
    }
    
    for (int i = 0; i < depth; i++)
        std::cout << "  ";

    const Vector3 size = node.box.size();
    const Vector3 center = node.box.center();

    std::cout << "Node(depth=" << depth
        << ", tris=" << node.triangles.size()
        << ", center=[" << center.x() << ", " << center.y() << ", " << center.z() << "]"
        << ", size=[" << size.x() << ", " << size.y() << ", " << size.z() << "])";

    
    if (node.triangles.size() > 8) std::cout << " <--- HEAVY";
    if (node.triangles.empty() && !node.childA && !node.childB) std::cout << " <--- USELESS LEAF";

    std::cout << "\n";

    if (node.childA)
        printNode(*node.childA, depth + 1, emptyCount, heavyCount);

    if (node.childB)
        printNode(*node.childB, depth + 1, emptyCount, heavyCount);
}
