#include <iostream>
#include <ostream>
#include <iosfwd>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>
#include <chrono>
#include <algorithm>

#include "../src/vector.h"
#include "../src/scene.h"
#include "../src/concurrency.h"
#include "../src/utils.h"

struct Ray
{
	Vector3 origin;
	Vector3 direction;
};

float srgb( float x )
{
	return std::pow( x, 1.f / 2.2f );
}

Vector3 tonemapping( const Vector3& color )
{
	return Vector3( std::min( 1.0f, color.x() ), std::min( 1.0f, color.y() ), std::min( 1.0f, color.z() ) );
}

Vector3 tonemappingUncharted( const Vector3& color )
{
	const Vector3 A = Vector3( 0.15f, 0.15f, 0.15f );
	const Vector3 B = Vector3( 0.50f, 0.50f, 0.50f );
	const Vector3 C = Vector3( 0.10f, 0.10f, 0.10f );
	const Vector3 D = Vector3( 0.20f, 0.20f, 0.20f );
	const Vector3 E = Vector3( 0.02f, 0.02f, 0.02f );
	const Vector3 F = Vector3( 0.30f, 0.30f, 0.30f );	
	const Vector3 wPoint = Vector3(11.20f, 11.30f, 11.20f);

	auto applay = [&](const Vector3& c) {
		return ((c * (A * c + C * B) + D * E) / (c * (A * c + B) + D * F)) - E / F;
		};	
	
	return applay(color) * (Vector3(1.0, 1.0f, 1.0f) / applay(wPoint));
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
				Vector3 color = tonemappingUncharted( data[y * width + x] );
				int r = (int)std::clamp( srgb(color.x()) * 255, 0.0f, 255.0f); // R
				int g = (int)std::clamp( srgb(color.y()) * 255, 0.0f, 255.0f); // G
				int b = (int)std::clamp( srgb(color.z()) * 255, 0.0f, 255.0f); // B

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

float intersectPlane2( const Ray& ray, const Vector3& normal, float d, float tMin, float tMax )
 {
	const float dist = dot( normal, ray.origin ) - d;
	const float dotND = dot( ray.direction, normal );
	if ( dotND == 0.0 )
	{
		if ( dist == 0.0 && tMin == 0.0)
		{
			return 0.0;
		}
		return tMax;
	}
	const float t = dist / -dotND;
	if ( t< tMin || t > tMax )
		return tMax;
	return t;
}

float intersectTriangle( const Ray& ray, const Vector3& a, const Vector3& b, const Vector3& c, float tMin, float tMax )
{
	const Vector3 normal = unit_vector( cross( b- a, c - a ) );
	const float d = dot( normal, a );
	const float t = intersectPlane2( ray, normal, d, tMin, tMax );
	if ( t == tMax )
		return tMax;
	const Vector3 p = ray.origin + ray.direction * t;
	if ( dot( cross( b - a, p - a ), normal ) < 0.0f )
		return tMax;
	if ( dot( cross( c - b, p - b ), normal ) < 0.0f )
		return tMax;
	if ( dot( cross( a - c, p - c ), normal ) < 0.0f )
		return tMax;
	return t;
}

float intersectSphere(const Ray& ray, const Vector3& center, float radius, float tMin, float tMax)
{
	const Vector3 origin = ray.origin - center; // сдвигаем сферу в центр 
	const float A = 1;
	const float B = 2.0f * dot( origin, ray.direction );
	const float C = dot( origin, origin ) - radius * radius;
	const float D = B * B - 4 * A * C;
	if ( D < 0.0f )
		return tMax;
	const float sqrtD = std::sqrt( D );
	const float t0 = ( -B - sqrtD ) / ( 2.0f * A );	
	if ( t0 >= tMin && t0 < tMax ) return t0;
	const float t1 = (-B + sqrtD) / (2.0f * A);
	if ( t1 >= tMin && t1 < tMax ) return t1;
	return tMax;
}

//Vector3 getUniformSampleOffset( int index, int side_count )
//{
//	const float haflDist = 0.5 / side_count;
//	const float dist = 1.0 / side_count;
//	const float x = index % side_count;
//	const float y = std::floor( index / side_count );
//	return Vector3( haflDist + x * dist, haflDist + y * dist, 0.0 );
//}

Vector3 getUniformSampleOffset(int index, int side_count)
{	
	const float x_idx = (float)(index % side_count);
	const float y_idx = (float)std::floor(index / side_count);
		
	const float dist = 1.0f / side_count;
		
	const float jitterX = randomFloat();
	const float jitterY = randomFloat();
		
	const float u = (x_idx + jitterX) * dist;
	const float v = (y_idx + jitterY) * dist;
	
	return Vector3(u, v, 0.0f);
}


Vector3 randomUniformVectorHemispher()
{
	float phi = randFloat(0, 1) * 2.0f * PI;
	float cosTheta = randFloat(0, 1) * 2.0f - 1.0f;
	float sinTheta = std::sqrt( 1 - cosTheta * cosTheta );
	float x = std::cos( phi ) * sinTheta;
	float y = cosTheta;
	float z = std::sin( phi ) * sinTheta;
	return Vector3( x, y, z );
}

Vector3 randOnHemispher(const Vector3& normal)
{
	Vector3 onSphere = randUnitVector();
	if (dot(onSphere, normal) > 0.0f)
		return onSphere;
	else
		return -onSphere;
}

Vector3 reflect(const Vector3& d, const Vector3& n)
{
	return d - 2.0f * dot(d, n) * n;
}

Vector3 trace( const Ray& ray, const Scene& scene, int depth )
{
	const float tMin = 0.001f;
	float tMax = 10000;
	Vector3 hitNormal;
	
	int matIndex = 0;
	for ( const auto& sp : scene.spheres() )
	{
		float t = intersectSphere( ray, sp.pos, sp.radius, tMin, tMax );
		if ( t < tMax )
		{
			Vector3 pos = ray.origin + ray.direction * t;
			hitNormal = unit_vector( pos - sp.pos );
			tMax = t;
			matIndex = sp.matIndex;
		}
	}

	for ( const auto& p : scene.planes() )
	{
		float t = intersectPlane2( ray, p.normal, p.dist, tMin, tMax );
		if ( t < tMax )
		{
			hitNormal = p.normal;
			tMax = t;
			matIndex = p.matIndex;
		}
	}
	for ( const auto& tr : scene.triangles() )
	{
		float t = intersectTriangle( ray, tr.a, tr.b, tr.c, tMin, tMax );
		if ( t < tMax )
		{
			hitNormal = unit_vector( cross( tr.b - tr.a, tr.c - tr.a ) );
			tMax = t;
			matIndex = tr.matIndex;
		}
	}
	if ( tMax == 10000 )
		return scene.enviroment();

	if ( dot( hitNormal, ray.direction ) > 0.0 )
		hitNormal = -hitNormal;

	const Material m = scene.materials()[ matIndex ];
	Vector3 newDir;
	float brdf = 1.0f / PI;
	float pdf = 1.0f / ( 2.0f * PI );
	float cosTheta; 

	if ( m.type == 0 )
	{
		newDir = randomUniformVectorHemispher();
		cosTheta = dot(newDir, hitNormal);
		if (cosTheta < 0.0)
			newDir *= -1;
	}
	else if ( m.type == 1 )
	{
		newDir = reflect( ray.direction, hitNormal );
		cosTheta = 1.0f;
		brdf = 1.0f;
		pdf = 1.0f;
		// cosTheta -> 0
	}
	
	//const Vector3 newDir = randOnHemispher( hitNormal );
	
	const Vector3 newOrig = ray.origin + ray.direction * tMax + newDir * 1e-4f;	
	const Ray newRay( {newOrig, newDir } );
	
	Vector3 color;
	if (depth > 4)
	{
		return scene.enviroment();
	}
	//	float p = randFloat( 0, 1 );
	//	if ( p <= 0.5 )
	//	{
	//		color = m.emmision;
	//	}
	//	else
	//	{
	//		color = trace( newRay, scene, depth + 1 ) * brdf * std::abs( cosTheta ) / pdf * m.albedo + m.emmision;
	//		color *= 1.0f / ( 1.0f - p );
	//	}
	//}
	//else
	{
		color = trace( newRay, scene, depth + 1 ) * brdf * std::abs( cosTheta ) / pdf * m.albedo + m.emmision;
	}

	return color;
}

Vector3 trace_iterative(Ray ray, const Scene& scene, int maxDepth)
{
	Vector3 throughput = Vector3(1.0, 1.0, 1.0);
	Vector3 radiance = Vector3(0.0, 0.0, 0.0);

	const float tMin = 0.001f;

	for (int depth = 0; depth < maxDepth; ++depth)
	{
		float tMax = 10000.0f;
		float t; 
		Vector3 hitNormal;
		int matIndex = -1; 

		
		for (const auto& sp : scene.spheres())
		{
			t = intersectSphere(ray, sp.pos, sp.radius, tMin, tMax);
			if (t < tMax)
			{
				Vector3 pos = ray.origin + ray.direction * t;
				hitNormal = unit_vector(pos - sp.pos);
				tMax = t;
				matIndex = sp.matIndex;
			}
		}

		for (const auto& p : scene.planes())
		{
			t = intersectPlane2(ray, p.normal, p.dist, tMin, tMax);
			if (t < tMax)
			{
				hitNormal = p.normal;
				tMax = t;
				matIndex = p.matIndex;
			}
		}

		for (const auto& tr : scene.triangles())
		{
			t = intersectTriangle(ray, tr.a, tr.b, tr.c, tMin, tMax);
			if (t < tMax)
			{
				hitNormal = unit_vector(cross(tr.b - tr.a, tr.c - tr.a));
				tMax = t;
				matIndex = tr.matIndex;
			}
		}

		if (matIndex == -1 || tMax == 10000.0f) // matIndex == -1 - более явная проверка на промах
		{
			radiance += throughput * scene.enviroment();
			break;
		}		
		
		if (dot(hitNormal, ray.direction) > 0.0)
			hitNormal = -hitNormal;

		
		Vector3 newDir = randomUniformVectorHemispher();

		auto cosTheta = dot(newDir, hitNormal);
		if (cosTheta < 0.0)
			newDir *= -1;

		cosTheta = dot(newDir, hitNormal);

		const Material m = scene.materials()[matIndex];

		
		radiance += throughput * m.emmision;

		const float brdf = 1.0f / PI;
		const float pdf = 1.0f / (2.0f * PI);

		throughput = throughput * m.albedo * (brdf * cosTheta / pdf);

		
		const Vector3 newOrig = ray.origin + ray.direction * tMax + newDir * 1e-4f;

		ray.origin = newOrig;
		ray.direction = newDir;
	}

	return radiance;
}

int main()
{
	Scene scene;
	//scene.load( "../scenes/02-scene-hard-v2.txt" );
	//scene.load( "../scenes/03-scene-hard.txt" );
	//scene.load( "../scenes/03-scene-easy.txt" );
	//scene.load( "../scenes/04-scene-easy.txt" );
	scene.load( "../scenes/04-scene-medium.txt" );

	const std::uint16_t width = scene.width();
	const std::uint16_t height = scene.height();
	const float aspectRatio = float(width) / height;
	auto& camera = scene.camera();
	Vector3 camerForward = unit_vector( camera.target - camera.pos );
	Vector3 camerRight = unit_vector(cross( camera.up, camerForward ));
	Vector3 camerUp =  cross( camerForward, camerRight );
		
	const float pixSize = 1.0f / height;
	const float viewportHight = 2.0f * std::tan( (camera.fov / 180.0f * PI) * 0.5f );

//	const Vector3 leftTop( -aspectRatio / 2, 0.5f, 1.0f );
	const Vector3 leftTop( -aspectRatio * viewportHight / 2.0f, viewportHight / 2.0f, 1.0f);

	std::vector<Vector3> data;
	data.resize( width * height );

	const int SIDE_SAMPLE_COUNT = scene.samples();
	auto start = std::chrono::high_resolution_clock::now();

	TaskManager manager(8, 32);

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			while (!manager.add([&](int x, int y, std::vector<Vector3>& data, const Scene& scene) {
				Vector3 color(0, 0, 0);
				const float u = float(x) / width;
				const float v = float(y) / height;

				for (int s = 0; s < SIDE_SAMPLE_COUNT * SIDE_SAMPLE_COUNT; ++s)
				{
					//Vector3 pixPos = leftTop + Vector3( pixSize / 2.0 + x * pixSize, pixSize / 2.0 + y * pixSize, 0 );
					//Vector3 pixPos = leftTop + Vector3( pixSize / 2.0f + u * aspectRatio, -pixSize / 2.0f - v, 0.0f );
					const Vector3 offset = getUniformSampleOffset(s, SIDE_SAMPLE_COUNT);
					const Vector3 pixPosVS = leftTop + Vector3((pixSize * offset.x() + u * aspectRatio) * viewportHight, (-pixSize * offset.y() - v) * viewportHight, 0.0f);
					const Vector3 pixPos = camera.pos + pixPosVS.x() * camerRight + pixPosVS.y() * camerUp + pixPosVS.z() * camerForward;

					const Vector3 dir = unit_vector(pixPos - camera.pos);
					const Ray ray({ camera.pos, dir });
					//color += trace_iterative( ray, scene, 4 );
					color += trace(ray, scene, 0);
				}

				data[y * width + x] = color / float(SIDE_SAMPLE_COUNT * SIDE_SAMPLE_COUNT);
				}, x, y, std::ref(data), scene)
			){}
		}
	}
	manager.stop();

	auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
	std::cout << "Time: " << duration_ms.count() << " milliseconds" << std::endl;

	saveImageToFile( width, height, data );

	return 0;
}