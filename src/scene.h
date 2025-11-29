#pragma once 

#include "vector.h"

#include <vector>

struct Sphere
{
	Vector3 pos;
	float radius;
	int matIndex;
};

struct Plane
{
	Vector3 normal;	
	float dist;
	int matIndex;
};

struct Triangle
{
	Vector3 a;
	Vector3 b;
	Vector3 c;
	int matIndex;
};

struct Material
{
	Vector3 albedo;
	Vector3 emmision;
	int type;
};

struct Camera
{
	Vector3 pos;
	Vector3 target;
	Vector3 up;
	float fov;
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
	Vector3 enviroment() const { return enviroment_; }
	

	const Camera& camera() const { return camera_; }

	const std::vector<Sphere>& spheres() const { return spheres_; }
	const std::vector<Plane>& planes() const { return planes_; }
	const std::vector<Triangle>& triangles() const { return triangles_; }
	const std::vector<Material>& materials() const { return materials_; }

	size_t count() const { return spheres_.size() + planes_.size(); }

private:
	void parse( const std::string& filename );	

private:
	int version_;
	int samples_;
	int width_;
	int height_;
	Camera camera_;
	Vector3 enviroment_;
	std::vector<Material> materials_;
	std::vector<Sphere> spheres_;
	std::vector<Plane> planes_;
	std::vector<Triangle> triangles_;
};
