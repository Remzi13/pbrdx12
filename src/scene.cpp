#include "scene.h"

#include <string>
#include <sstream>
#include <iostream>
#include <ostream>
#include <iosfwd>
#include <fstream>

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
}


Scene::Scene()
{
	
}

bool Scene::load( const char* name )
{
	parse( name );
	return true;
}

void Scene::parse( const std::string& filename ) {
	std::ifstream file( filename );
	if ( !file.is_open() ) {
		std::cerr << "Ошибка: Не удалось открыть файл " << filename << std::endl;
		return;
	}
	// 1. Читаем настройки сцены (width, height, samples)
	std::stringstream ss = getNextDataLine( file );
	if ( ss.fail() )
		return;
	ss >> version_;

	ss = getNextDataLine( file );

	ss >> width_ >> height_ >> samples_;

	// 2. Читаем Сферы
	ss = getNextDataLine( file );
	int numSpheres;
	ss >> numSpheres;

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

	// 3. Читаем Плоскости
	ss = getNextDataLine( file );
	int numPlanes;
	ss >> numPlanes;

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

	// 4. Читаем теругольники
	ss = getNextDataLine( file );
	int numTriangles;
	ss >> numTriangles;

	for ( int i = 0; i < numTriangles; ++i ) {
		ss = getNextDataLine( file );

		float x1, y1, z1, x2, y2, z2, x3, y3, z3, r, g, b;
		ss >> x1 >> y1 >> z1 >> x2 >> y2 >> z2 >> x3 >> y3 >> z3  >> r >> g >> b;

		Triangle t;
		t.a = Vector3( x1, y1, z1  );
		t.b = Vector3( x2, y2, z2  );
		t.c = Vector3( x3, y3, z3  );
		t.color = Vector3( r, g, b );
		triangles_.push_back( t );
	}
}