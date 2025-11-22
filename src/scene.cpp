#include "scene.h"

#include <string>
#include <sstream>
#include <iostream>
#include <ostream>
#include <iosfwd>
#include <fstream>
#include <algorithm>

namespace {
	std::stringstream getNextDataLine( std::ifstream& file )
	{
		std::string line;
		while ( std::getline( file, line ) ) {
			size_t firstChar = line.find_first_not_of( " \t\r\n" );

			if ( firstChar == std::string::npos || line[firstChar] == '#' ) {
				continue;
			}
			return std::stringstream( line );
		}
		return std::stringstream( "" );
	}

	float intersectSphere(const Ray& ray, const Vector3& center, float radius, float tMin, float tMax)
	{
		const Vector3 origin = ray.origin - center; // сдвигаем сферу в центр 
		const float A = 1;
		const float B = 2.0 * dot(origin, ray.direction);
		const float C = dot(origin, origin) - radius * radius;
		const float D = B * B - 4 * A * C;
		if (D < 0.0)
			return tMax;
		const float sqrtD = std::sqrt(D);
		const float t0 = (-B - sqrtD) / (2.0 * A);
		if (t0 >= tMin && t0 < tMax) return t0;
		const float t1 = (-B + sqrtD) / (2.0 * A);
		if (t1 >= tMin && t1 < tMax) return t1;
		return tMax;
	}

	float intersectPlane2(const Ray& ray, const Vector3& normal, float d, float tMin, float tMax)
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

	float intefsectTriangle(const Ray& ray, const Vector3 a, const Vector3& b, const Vector3& c, float tMin, float tMax)
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


Scene::Scene()
{	
}

bool Scene::load( const char* name )
{
	parse( name );
	return true;
}

Vector3 farPointZ(const Triangle& t)
{
	Vector3 f = t.a;
	if (f.z() > t.b.z())
		f = t.b;
	if (f.z() > t.c.z())
		f = t.c;
	return f;
}

void Scene::parse( const std::string& filename ) {
	std::ifstream file( filename );
	if ( !file.is_open() ) {
		std::cerr << "Could not open file" << filename << std::endl;
		return;
	}
	// 1. ×èòàåì íàñòðîéêè ñöåíû (width, height, samples)
	std::stringstream ss = getNextDataLine( file );
	if ( ss.fail() )
		return;

	ss >> version_;
	ss = getNextDataLine(file);

	ss >> width_ >> height_ >> samples_;

	// 2. ×èòàåì Ñôåðû
	ss = getNextDataLine( file );
	int numSpheres;
	ss >> numSpheres;
	spheres_.reserve(numSpheres);
	for ( int i = 0; i < numSpheres; ++i ) {
		ss = getNextDataLine( file );
		Sphere sphere;
		float x, y, z, rad, r, g, b;
		ss >> x >> y >> z >> rad >> r >> g >> b;
		sphere.pos = Vector3( x, y, z );
		sphere.color = Vector3( r, g, b );
		sphere.radius = rad;

		spheres_.push_back( sphere );
	}

	// 3. ×èòàåì Ïëîñêîñòè
	ss = getNextDataLine( file );
	int numPlanes;
	ss >> numPlanes;
	planes_.reserve(numPlanes);
	for ( int i = 0; i < numPlanes; ++i ) {
		ss = getNextDataLine( file );
		Plane p;
		float x, y, z, dist, r, g, b;
		ss >> x >> y >> z >> dist >> r >> g >> b;
		p.normal = Vector3( x, y, z );
		p.dist = dist;
		p.color = Vector3( r, g, b );
		planes_.push_back( p );
	}

	// 4. Triangles 
	ss = getNextDataLine(file);
	int numTriangles;
	ss >> numTriangles;
	triangles_.reserve(numTriangles);
	for (int i = 0; i < numTriangles; ++i) {
		ss = getNextDataLine(file);
		
		float x1, y1, z1, x2, y2, z2, x3, y3, z3, r, g, b;
		ss >> x1 >> y1 >> z1 >> x2 >> y2 >> z2 >> x3 >> y3 >> z3 >> r >> g >> b;
		Triangle tr;
		tr.a = Vector3(x1, y1, z1);
		tr.b = Vector3(x2, y2, z2);
		tr.c = Vector3(x3, y3, z3);
		tr.color = Vector3(r, g, b);

		triangles_.push_back(tr);
	}

	std::sort(triangles_.begin(), triangles_.end(), [](const Triangle& a, const Triangle& b) {
		const Vector3 farA = farPointZ(a);
		const Vector3 farB = farPointZ(b);
		return farA.z() < farB.z();
	});
}

HitInfo Scene::rayCast(const Ray& ray, float min, float max) const
{	
	float t = max;
	Vector3 color;
	Vector3 normal;
	for (const auto& sp : spheres_)
	{
		t = intersectSphere(ray, sp.pos, sp.radius, min, max);
		if (t < max)
		{
			color = sp.color;
			const Vector3 point = ray.origin + ray.direction * t;
			normal = unit_vector(point - sp.pos);
			max = t;
		}
	}

	for (const auto& p : planes_)
	{
		t = intersectPlane2(ray, p.normal, p.dist, min, max);
		if (t < max)
		{
			color = p.color;
			normal = p.normal;
			max = t;
		}
	}

	for (const auto& tr : triangles_)
	{
 		t = intefsectTriangle(ray, tr.a, tr.b, tr.c, min, max);
		if (t < max)
		{
			color = tr.color;
			normal = unit_vector(cross(tr.b - tr.a, tr.c - tr.a));
			max = t;
			Vector3 far = farPointZ(tr);
			if (t > far.z())
				break;
		}
		
	}

	if (dot(normal, ray.direction) > 0.0)
		normal = -normal;

	return { color, normal, t };
}