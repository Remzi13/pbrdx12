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

class Scene
{
public:
	Scene();

	bool load( const char* name );

	void setSamples( int i ) { samples_ = i; }

	int samples() const { return samples_; }
	int width() const { return width_; }
	int height() const { return height_; }

	const std::vector<Sphere>& spheres() const { return spheres_; }
	const std::vector<Plane>& planes() const { return planes_; }

	size_t count() const { return spheres_.size() + planes_.size(); }

private:
	void parse( const std::string& filename );	

private:
	int samples_;
	int width_;
	int height_;
	std::vector<Sphere> spheres_;
	std::vector<Plane> planes_;
};
