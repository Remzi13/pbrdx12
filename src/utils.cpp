#include <random>

#include "vector.h"

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
