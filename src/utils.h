#pragma once

#include "vector.h"

constexpr float PI = 3.14f;

namespace math {
	struct Triangle
	{
		Vector3 a;
		Vector3 b;
		Vector3 c;
		int matIndex;
	};


	Vector3 triangleCenter(const Triangle& triangle);

	struct Ray
	{
		Vector3 origin;
		Vector3 direction;
	};

	float intersectPlane2(const Ray& ray, const Vector3& normal, float d, float tMin, float tMax);
	float intersectTriangle(const Ray& ray, const Vector3& a, const Vector3& b, const Vector3& c, float tMin, float tMax);
}
float randomFloat();
float randFloat(float min, float max);
Vector3 randUnitVector();
Vector3 randVector(float min, float max);