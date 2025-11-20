#pragma once 

#include "vector.h"

#include <vector>

struct Sphere
{
	Vector3 pos;
	Vector3 color;
	float radius;
};

struct Plane
{
	Vector3 normal;
	Vector3 color;
	float dist;
};

struct HitInfo
{
	Vector3 color;
	Vector3 normal;
	float dist;
};

struct Ray
{
	Vector3 origin;
	Vector3 direction;
};


class Scene
{
public:
	Scene();

	bool load( const char* name );

	void setSamples( int i ) { samples_ = i; }

	int samples() const { return samples_; }
	int width() const { return width_; }
	int height() const { return height_; }


	HitInfo rayCast(const Ray& ray, float min, float max) const;

private:
	void parse( const std::string& filename );	

private:
	int samples_;
	int width_;
	int height_;
	std::vector<Sphere> spheres_;
	std::vector<Plane> planes_;
};
