#include "utils.h"

#include <random>

namespace math {
	Vector3 triangleCenter(const Triangle& triangle)
	{
		return (triangle.a + triangle.b + triangle.c) / 3.0f;
	}

	float intersectPlane2(const math::Ray& ray, const Vector3& normal, float d, float tMin, float tMax)
	{
		const float dist = dot(normal, ray.origin) - d;
		const float dotND = dot(ray.direction, normal);
		if (dotND == 0.0)
		{
			if (dist == 0.0 && tMin == 0.0)
			{
				return 0.0;
			}
			return tMax;
		}
		const float t = dist / -dotND;
		if (t< tMin || t > tMax)
			return tMax;
		return t;
	}

	float intersectTriangle(const math::Ray& ray, const Vector3& a, const Vector3& b, const Vector3& c, float tMin, float tMax)
	{
		const Vector3 normal = unit_vector(cross(b - a, c - a));
		const float d = dot(normal, a);
		const float t = intersectPlane2(ray, normal, d, tMin, tMax);
		if (t == tMax)
			return tMax;
		const Vector3 p = ray.origin + ray.direction * t;
		if (dot(cross(b - a, p - a), normal) < 0.0f)
			return tMax;
		if (dot(cross(c - b, p - b), normal) < 0.0f)
			return tMax;
		if (dot(cross(a - c, p - c), normal) < 0.0f)
			return tMax;
		return t;
	}
}

float randomFloat()
{
	static std::uniform_real_distribution<float> distribution(0.0, 1.0);
	static std::mt19937 generator;
	return distribution(generator);
}

float randFloat(float min, float max)
{
	return min + (max - min) * (randomFloat());
}

Vector3 randVector(float min, float max)
{
	return Vector3(randFloat(min, max), randFloat(min, max), randFloat(min, max));
}

Vector3 randUnitVector()
{
	while (true)
	{
		Vector3 p = randVector(-1, 1);
		float l = p.length_squared();
		if (1e-160 < l && l <= 1)
			return p / std::sqrt(l);
	}
}
