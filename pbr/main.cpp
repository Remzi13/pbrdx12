#include <iostream>
#include <ostream>
#include <iosfwd>
#include <string>
#include <fstream>
#include <vector>
#include <math.h>
#include <sstream>
#include <chrono>

#include "../src/vector.h"

struct Ray
{
	Vector3 origin;
	Vector3 direction;
};

float srgb( float x )
{
	return std::pow( x, 1 / 2.2 );
}

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
	Scene(const char* name ) 
	{
		parse(name);
	}


	int samples() const { return samples_; }
	int width() const { return width_; }
	int height() const { return height_; }

	const std::vector<Sphere>& spheres() const { return spheres_; }
	const std::vector<Plane>& planes() const { return planes_; }

private:

	std::stringstream getNextDataLine(std::ifstream& file)
	{
		std::string line;
		while (std::getline(file, line)) {
			size_t firstChar = line.find_first_not_of(" \t\r\n");

			if (firstChar == std::string::npos || line[firstChar] == '#') {
				continue;
			}
			return std::stringstream(line);
		}
		return std::stringstream("");
	}

	void parse(const std::string& filename) {
		std::ifstream file(filename);
		if (!file.is_open()) {
			std::cerr << "Ошибка: Не удалось открыть файл " << filename << std::endl;
			return;
		}
		// 1. Читаем настройки сцены (width, height, samples)
		std::stringstream ss = getNextDataLine(file);
		if (ss.fail())
			return;

		ss >> width_ >> height_ >> samples_;

		// 2. Читаем Сферы
		ss = getNextDataLine(file);
		int numSpheres;
		ss >> numSpheres;

		for (int i = 0; i < numSpheres; ++i) {
			ss = getNextDataLine(file);
			Sphere sphere;
			float x, y, z, rad, r, g, b;
			ss >> x >> y >> z >> rad >> r >> g >> b;
			sphere.pos = Vector3(x, y, z);
			sphere.color = Vector3(r, g, b);
			sphere.radius = rad;

			spheres_.push_back(sphere);
		}

		// 3. Читаем Плоскости
		ss = getNextDataLine(file);
		int numPlanes;
		ss >> numPlanes;

		for (int i = 0; i < numPlanes; ++i) {
			ss = getNextDataLine(file);
			Plane p;
			float x, y, z, dist, r, g, b;
			ss >> x >> y >> z >> dist >> r >> g >> b;
			p.normal = Vector3(x, y, z);
			p.dist = dist;
			p.color = Vector3(r, g, b);
			planes_.push_back(p);
		}
	}

private:
	int samples_;
	int width_;
	int height_;
	std::vector<Sphere> spheres_;
	std::vector<Plane> planes_;
};





void saveImageToFile( std::uint16_t width, std::uint16_t height, const std::vector<Vector3>& data )
{
	std::ofstream outfile( "output.ppm", std::ios::out | std::ios::binary );

	if ( outfile.is_open() )
	{
		outfile << "P3\n" << width << " " << height << "\n255\n";
		
		for ( int y = 0; y < height; ++y )
		{
			for ( int x = 0; x < width; ++x )
			{

				int r = srgb(data[y * width + x].x()) * 255; // R
				int g = srgb(data[y * width + x].y()) * 255; // G
				int b = srgb(data[y * width + x].z()) * 255; // B

				outfile << r << " " << g << " " << b << " ";
				
			}
			outfile << "\n";
		}
		outfile.close();

		// Сообщаем о сохранении
		printf( "Image saved to output.ppm\n" );
	}
	else
	{
		printf( "Error: Could not open output.ppm for writing.\n" );
	}
}

//можно использовать точку и дистанцию 
float intersectPlane( Ray ray,  Vector3 poinOnPlane, Vector3 normPlane, float tMin, float tMax )
{
	float t =  dot( (poinOnPlane - ray.origin ) , normPlane ) / dot( ray.direction , normPlane );
	if ( t > tMin && t < tMax )
	{
		return t;
	}
	else
	{
		return tMax;
	}
}

float intersectPlane2( Ray ray, Vector3 normal, float d, float tMin, float tMax )
 {
	float dist = dot( normal, ray.origin ) - d;
	float dotND = dot( ray.direction, normal );
	if ( dotND == 0.0 )
	{
		if ( dist == 0.0 && tMin == 0.0)
		{
			return 0.0;
		}
		return tMax;
	}
	float t = dist / -dotND;
	if ( t< tMin || t > tMax )
		return tMax;
	return t;
}

float intersectSphere(Ray ray, Vector3 center, float radius, float tMin, float tMax)
{
	const Vector3 origin = ray.origin - center; // сдвигаем сферу в центр 
	const float A = 1;
	const float B = 2.0 * dot( origin, ray.direction );
	const float C = dot( origin, origin ) - radius * radius;
	const float D = B * B - 4 * A * C;
	if ( D < 0.0 )
		return tMax;
	const float sqrtD = std::sqrt( D );
	const float t0 = ( -B - sqrtD ) / ( 2.0 * A );
	const float t1 = ( -B + sqrtD ) / ( 2.0 * A );
	if ( t0 >= tMin && t0 < tMax )
		return t0;
	if ( t1 >= tMin && t1 < tMax )
		return t1;
	return tMax;
}

Vector3 getUniformSampleOffset( int index, int side_count )
{
	const float haflDist = 0.5 / side_count;
	const float dist = 1.0 / side_count;
	const float x = index % side_count;
	const float y = std::floor( index / side_count );
	return Vector3( haflDist + x * dist, haflDist + y * dist, 0.0 );
}


int main()
{
	Scene scene("scene.txt");

	const std::uint16_t width = scene.width();
	const std::uint16_t height = scene.height();
	const float aspectRatio = float(width) / height;
	Vector3 cameraOrigin( 0, 0, 0 );
	float pixSize = 1.0f / height;
	Vector3 leftTop( -aspectRatio / 2, 0.5f, 1.0f );
	std::vector<Vector3> data;
	data.resize( width * height );
	int SIDE_SAMPLE_COUNT = scene.samples();
	auto start = std::chrono::high_resolution_clock::now();

	for ( int y = 0; y < height; ++y )
	{
		for ( int x = 0; x < width; ++x )
		{
			Vector3 color( 0, 0, 0);
			for ( int s = 0; s < SIDE_SAMPLE_COUNT * SIDE_SAMPLE_COUNT; ++s )
			{
				const float u = float( x ) / width;
				const float v = float( y ) / height;
				//Vector3 pixPos = leftTop + Vector3( pixSize / 2.0 + x * pixSize, pixSize / 2.0 + y * pixSize, 0 );
				const Vector3 offset = getUniformSampleOffset( s, SIDE_SAMPLE_COUNT );
				const Vector3 pixPos = leftTop + Vector3( pixSize * offset.x() + u * aspectRatio, -pixSize * offset.y() - v, 0.0f );

				const Vector3 dir = unit_vector( pixPos - cameraOrigin );

				Vector3 c = Vector3( dir.x() * 0.5 + 0.5, dir.y() * 0.5 + 0.5, dir.z() * 0.5 + 0.5 );
				
				const float tMin = 0.001f;
				float tMax = 10000;
				const Ray ray( { cameraOrigin, dir } );

				float t = 0;
				for ( auto sp : scene.spheres() )
				{
					float t = intersectSphere( ray, sp.pos, sp.radius, tMin, tMax );
					if ( t < tMax )
					{
						c = sp.color;
						tMax = t;
					}
				}

				for ( auto p : scene.planes() )
				{
					t = intersectPlane2( ray, p.normal, p.dist, tMin, tMax );
					if ( t < tMax )
					{
						c = p.color;
					}
				}
				color += c;
			}

			data[y * width + x] = color / float(SIDE_SAMPLE_COUNT * SIDE_SAMPLE_COUNT);
		}
	}
		
	auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
	std::cout << "Time: " << duration_ms.count() << " milliseconds" << std::endl;

	saveImageToFile( width, height, data );

	return 0;
}