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
#include "../src/scene.h"


float srgb( float x )
{
	return std::pow( x, 1 / 2.2 );
}

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
	Scene scene;
	scene.load( "../scene.txt" );

	const std::uint16_t width = scene.width();
	const std::uint16_t height = scene.height();
	const float aspectRatio = float(width) / height;
	const Vector3 cameraOrigin( 0, 0, 0 );
	const float pixSize = 1.0f / height;
	const Vector3 leftTop( -aspectRatio / 2, 0.5f, 1.0f );

	std::vector<Vector3> data;
	data.resize( width * height );

	const int SIDE_SAMPLE_COUNT = scene.samples();
	auto start = std::chrono::high_resolution_clock::now();

	for ( int y = 0; y < height; ++y )
	{
		for ( int x = 0; x < width; ++x )
		{
			Vector3 color( 0, 0, 0);
			const float u = float(x) / width;
			const float v = float(y) / height;
			const float tMin = 0.001f;

			for ( int s = 0; s < SIDE_SAMPLE_COUNT * SIDE_SAMPLE_COUNT; ++s )
			{
				//Vector3 pixPos = leftTop + Vector3( pixSize / 2.0 + x * pixSize, pixSize / 2.0 + y * pixSize, 0 );
				//Vector3 pixPos = leftTop + Vector3( pixSize / 2.0f + u * aspectRatio, -pixSize / 2.0f - v, 0.0f );
				const Vector3 offset = getUniformSampleOffset( s, SIDE_SAMPLE_COUNT );
				const Vector3 pixPos = leftTop + Vector3( pixSize * offset.x() + u * aspectRatio, -pixSize * offset.y() - v, 0.0f );

				const Vector3 dir = unit_vector( pixPos - cameraOrigin );

				Vector3 c = Vector3( dir.x() * 0.5 + 0.5, dir.y() * 0.5 + 0.5, dir.z() * 0.5 + 0.5 );
				
				float tMax = 10000;
				const Ray ray( { cameraOrigin, dir } );
				HitInfo hit = scene.rayCast(ray, tMin, tMax );
				if (hit.dist < tMax)
				{
					color += hit.color;
				}
				else
				{
					color += c;
				}
			}

			data[y * width + x] = color / float(SIDE_SAMPLE_COUNT * SIDE_SAMPLE_COUNT);
		}
	}
		
	auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
	std::cout << "Time: " << duration_ms.count() << " milliseconds" << std::endl;

	saveImageToFile( width, height, data );

	return 0;
}