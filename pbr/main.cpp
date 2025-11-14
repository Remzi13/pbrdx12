#include <iostream>
#include <ostream>
#include <iosfwd>
#include <string>
#include <fstream>
#include <vector>
#include <math.h>
#include <algorithm>

#include "../src/vector.h"

struct Ray
{
	Vector3 origin;
	Vector3 direction;
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

				int r = data[y * width + x].x() * 255; // R
				int g = data[y * width + x].y() * 255; // G
				int b = data[y * width + x].z() * 255; // B

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

float intersectSphere(Ray ray, Vector3 center, float radius, float tMin, float tMax)
{
	Vector3 origin = ray.origin - center; // сдвигаем сферу в центр 
	float A = 1;
	float B = 2.0 * dot( origin, ray.direction );
	float C = dot( origin, origin ) - radius * radius;
	float D = B * B - 4 * A * C;
	if ( D < 0.0 )
		return tMax;
	float sqrtD = std::sqrt( D );
	float t0 = ( -B - sqrtD ) / ( 2.0 * A );
	float t1 = ( -B + sqrtD ) / ( 2.0 * A );
	if ( t0 >= tMin && t0 < tMax )
		return t0;
	if ( t1 >= tMin && t1 < tMax )
		return t1;
	return tMax;
}

int main()
{
	const std::uint16_t width = 60;
	const std::uint16_t height = 40;
	const float aspectRatio = float(width) / height;
	Vector3 cameraOrigin( 0, 0, 0 );
	float pixSize = 1.0f / height;
	Vector3 leftTop( -aspectRatio / 2, 0.5f, 1.0f );
	std::vector<Vector3> data;
	data.resize( width * height );

	for ( int y = 0; y < height; ++y )
	{
		for ( int x = 0; x < width; ++x )
		{
			float u = float( x ) / width;
			float v = float( y ) / height;
			//Vector3 pixPos = leftTop + Vector3( pixSize / 2.0 + x * pixSize, pixSize / 2.0 + y * pixSize, 0 );
			Vector3 pixPos = leftTop + Vector3(
				pixSize / 2.0f + u * aspectRatio,
				-pixSize / 2.0f - v, 0.0f);

			Vector3 dir = unit_vector( pixPos - cameraOrigin );
			
			data[y * width + x] = Vector3( dir.x() * 0.5 + 0.5 , dir.y() * 0.5 + 0.5, dir.z() * 0.5 + 0.5);
			float tMin = 0.001;
			float tMax = 10000;
			Ray ray( { cameraOrigin, dir } );
			
			
			Vector3 normPlane( 0.0f, 1.0f, 0.0f );
			Vector3 point( 0.0f, -1.0f, 0.0f );
			float t = intersectPlane( { cameraOrigin, dir }, point, normPlane, tMin, tMax );
			if ( t <  tMax )
			{
				Vector3 pos = ray.origin + ray.direction * t;
				if ( ( int( pos.x() + 1000 ) % 2 ) != ( int( pos.z() + 1000 ) % 2 ) )
				{
					data[y * width + x] = Vector3( 1.0, 0.5, 0.5 );
				}
				else
				{
					data[y * width + x] = Vector3( 0.5, 0.5, 0.5 );
				}
				tMax = t;
			}

			t = intersectSphere( { cameraOrigin, dir }, Vector3( 0.0, -0.5, 7.0 ), 1.5, tMin, tMax );
			if ( t < tMax )
			{
				data[y * width + x] = Vector3( 0.5, 0.9, 0.5 );
				
			}
		}
	}
	std::cout << data[0] << "\n" << data[width * height - 1] << "\n";

	saveImageToFile( width, height, data );

	return 0;
}