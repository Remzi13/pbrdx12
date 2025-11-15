#include "app.h"

#include <iostream>
#include <ostream>
#include <iosfwd>
#include <string>
#include <fstream>
#include <math.h>
#include <algorithm>
#include <sstream>
#include <conio.h>

struct Ray
{
	Vector3 origin;
	Vector3 direction;
};


void saveImageToFile(std::uint16_t width, std::uint16_t height, const std::vector<Vector3>& data)
{
	std::ofstream outfile("output.ppm", std::ios::out | std::ios::binary);

	if (outfile.is_open())
	{
		outfile << "P3\n" << width << " " << height << "\n255\n";

		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
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
		printf("Image saved to output.ppm\n");
	}
	else
	{
		printf("Error: Could not open output.ppm for writing.\n");
	}
}

//можно использовать точку и дистанцию 
float intersectPlane(Ray ray, Vector3 poinOnPlane, Vector3 normPlane, float tMin, float tMax)
{
	float t = dot((poinOnPlane - ray.origin), normPlane) / dot(ray.direction, normPlane);
	if (t > tMin && t < tMax)
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
	float B = 2.0 * dot(origin, ray.direction);
	float C = dot(origin, origin) - radius * radius;
	float D = B * B - 4 * A * C;
	if (D < 0.0)
		return tMax;
	float sqrtD = std::sqrt(D);
	float t0 = (-B - sqrtD) / (2.0 * A);
	float t1 = (-B + sqrtD) / (2.0 * A);
	if (t0 >= tMin && t0 < tMax)
		return t0;
	if (t1 >= tMin && t1 < tMax)
		return t1;
	return tMax;
}

std::string renderFrameToString(std::uint16_t width, std::uint16_t height, const std::vector<Vector3>& data)
{
	// Используем stringstream для построения буфера в памяти
	std::stringstream buffer;

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			// ... [Ваш код для получения r, g, b и pixel] ...
			const Vector3& pixel = data[y * width + x];
			int r = static_cast<int>(std::max(0.0f, std::min(pixel.x() * 255.999f, 255.0f)));
			int g = static_cast<int>(std::max(0.0f, std::min(pixel.y() * 255.999f, 255.0f)));
			int b = static_cast<int>(std::max(0.0f, std::min(pixel.z() * 255.999f, 255.0f)));

			// Вместо printf используем buffer <<
			// Устанавливаем цвет фона:
			buffer << "\x1B[48;2;" << r << ";" << g << ";" << b << "m";

			// Печатаем "пиксель" (пробел):
			buffer << " ";

			// Сбрасываем цвет:
			buffer << "\x1B[0m";
		}
		// Новая строка в конце ряда:
		buffer << "\n";
	}

	return buffer.str();
}

void resetConsoleCursor()
{
	// \x1B[H - ANSI-код для перемещения курсора в 
	//         позицию 1,1 (верхний левый угол).
	printf("\x1B[H");
}

App::App(std::uint16_t width, std::uint16_t height) : width_(width), height_(height)
{ 
	data_.resize(width_ * height_);
}

bool App::isRun() const
{
	return isRun_;
}

void App::update()
{
	const float aspectRatio = float(width_) / height_;	
	float pixSize = 1.0f / height_;
	Vector3 leftTop(-aspectRatio / 2, 0.5f, cameraOrigin_.z() + 1.0f);
	
	const float consoleCharAspectRatio = 0.5f;
	const float verticalShift = (1.0f - consoleCharAspectRatio) / 2.0f;

	for (int y = 0; y < height_; ++y)
	{
		for (int x = 0; x < width_; ++x)
		{
			float u = float(x) / width_;
			float v = float(y) / height_;
			//Vector3 pixPos = leftTop + Vector3( pixSize / 2.0 + x * pixSize, pixSize / 2.0 + y * pixSize, 0 );
			Vector3 pixPos = leftTop + Vector3( pixSize / 2.0f + u * aspectRatio, -pixSize / 2.0f - (v * consoleCharAspectRatio) - verticalShift, 0.0f);

			Vector3 dir = unit_vector(pixPos - cameraOrigin_);

			data_[y * width_ + x] = Vector3(dir.x() * 0.5 + 0.5, dir.y() * 0.5 + 0.5, dir.z() * 0.5 + 0.5);
			float tMin = 0.001;
			float tMax = 10000;
			Ray ray({ cameraOrigin_, dir });


			Vector3 normPlane(0.0f, 1.0f, 0.0f);
			Vector3 point(0.0f, -1.0f, 0.0f);
			float t = intersectPlane({ cameraOrigin_, dir }, point, normPlane, tMin, tMax);
			if (t < tMax)
			{
				Vector3 pos = ray.origin + ray.direction * t;
				if ((int(pos.x() + 1000) % 2) != (int(pos.z() + 1000) % 2))
				{
					data_[y * width_ + x] = Vector3(1.0, 0.5, 0.5);
				}
				else
				{
					data_[y * width_ + x] = Vector3(0.5, 0.5, 0.5);
				}
				tMax = t;
			}

			t = intersectSphere({ cameraOrigin_, dir }, Vector3(0.0, -0.5, 7.0), 1.5, tMin, tMax);
			if (t < tMax)
			{
				data_[y * width_ + x] = Vector3(0.5, 0.9, 0.5);				
			}
		}
	}
	
	std::string frameBuffer = renderFrameToString(width_, height_, data_);
	resetConsoleCursor();
	// 3. Печатаем весь кадр ОДНИМ БЛОКОМ!
	std::cout << frameBuffer << std::flush;
}

void App::save() const
{
	saveImageToFile(width_, height_, data_);
}

void App::input()
{
	if (_kbhit())
	{	
		int inputKey = _getch();
	
		switch (inputKey)
		{		
		case 72:
			cameraOrigin_ = Vector3(cameraOrigin_.x(), cameraOrigin_.y(), cameraOrigin_.z() + 0.5);
			break;
		case 80:
			cameraOrigin_ = Vector3(cameraOrigin_.x(), cameraOrigin_.y(), cameraOrigin_.z() - 0.5);			
			break;
		}
	}
}