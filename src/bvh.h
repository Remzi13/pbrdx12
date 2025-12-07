#pragma once

#include "vector.h"
#include "utils.h"

#include <vector>

class BBox
{
public:
	BBox();

	const Vector3& min() const { return min_; }
	const Vector3& max() const { return max_; }

	void growTo(const Vector3& point);
	void growTo(const math::Triangle& t);

	Vector3 center() const;
	Vector3 size() const;

private:
	Vector3 min_;
	Vector3 max_;	
};

class BVH
{
public:

	void build(const std::vector<math::Triangle>& triangles);

	float intersect(const math::Ray& ray, float tMin, float tMax, math::Triangle& tr) const;

	void print() const;
	
private:
	struct Node
	{
		BBox box;
		std::vector<math::Triangle> triangles;
		std::unique_ptr<Node> childA;
		std::unique_ptr<Node> childB;
	};

	void split(Node& parent, int depth = 0) const;
	void printNode(const Node& node, int depth) const;
	float intersect(const math::Ray& ray, const Node& node, float tMin, float tMax, math::Triangle& tr) const;

	std::unique_ptr<Node> root_;
};