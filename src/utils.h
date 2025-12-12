#pragma once

#include "vector.h"

constexpr float PI = 3.14159265359f;
constexpr float INV_PI = 1.0f / PI;
constexpr float EPS = 0.00000001f;

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


	template <typename T>
	constexpr T saturate(T x) {
		return std::max(T(0.0), std::min(x, T(1.0)));
	}

	template <typename T, typename U>
	constexpr T lerp(const T& a, const T& b, const U& t) {
		return a + (b - a) * t;
	}

	float intersectPlane2(const Ray& ray, const Vector3& normal, float d, float tMin, float tMax);
	float intersectTriangle(const Ray& ray, const Vector3& a, const Vector3& b, const Vector3& c, float tMin, float tMax);
}
float randomFloat();
float randFloat(float min, float max);
Vector3 randUnitVector();
Vector3 randVector(float min, float max);