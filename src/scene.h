#pragma once 


#include "vector.h"
#include "bvh.h"

#include <vector>

struct Plane
{
	Vector3 normal;	
	float dist;
	int matIndex;
};

struct Material
{
	Vector3 albedo;
	Vector3 emission;
	int type;
	float metallic;
	float roughness;
};

struct Camera
{
	Vector3 pos;
	Vector3 target;
	Vector3 up;
	float fov;
	float aspectRatio;
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
	
	void addSphere(const math::Sphere& sp) { spheres_.push_back(sp); }

	void setCamera(const Camera& camera) { camera_ = camera; }
	const Camera& camera() const { return camera_; }

	const std::vector<math::Sphere>& spheres() const { return spheres_; }
	const std::vector<Plane>& planes() const { return planes_; }
	const std::vector<math::Triangle>& triangles() const { return triangles_; }
	const std::vector<Material>& materials() const { return materials_; }

	size_t count() const { return spheres_.size() + planes_.size(); }

	float intersect(const math::Ray& ray, float tMin, float tMax, math::Triangle& tr) const;
	float intersect(const math::Ray& ray, float tMin, float tMax, math::Sphere& sp) const;

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
	std::vector<math::Sphere> spheres_;
	std::vector<Plane> planes_;
	std::vector<math::Triangle> triangles_;

	BVH<math::Triangle> bvh_;
	BVH<math::Sphere> bvhSphere_;
};

class Scene2
{
	struct Node
	{
		std::string name;
		std::vector<math::Triangle> triangles_;
	};

public:
	void addNode(const std::string& name, const std::vector<math::Triangle>& triangles);
	void addMaterial( const Material& m );
	const std::vector<Material>& materials() const { return materials_; }

	float intersect( const math::Ray& ray, float tMin, float tMax, math::Triangle& tr ) const;

	void setCamera( const Camera& camera ) { camera_ = camera; }
	const Camera& camera() const { return camera_; }

private:
	Camera camera_;
	std::vector<Node> nodes_;
	std::vector<Material> materials_;

};