#include "app.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <conio.h>
#include <windows.h>
#undef max
#undef min
#include <algorithm>
#include <cmath>
#include <iomanip>


struct Ray
{
    Vector3 origin;
    Vector3 direction;
};

const float CONSOLE_CHAR_ASPECT_RATIO = 0.45f;
const float FOCAL_LENGTH = 1.0f;

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
                int r = (int)data[y * width + x].x() * 255;
                int g = (int)data[y * width + x].y() * 255;
                int b = (int)data[y * width + x].z() * 255;

                outfile << r << " " << g << " " << b << " ";
            }
            outfile << "\n";
        }
        outfile.close();
        printf("Image saved to output.ppm\n");
    }
    else
    {
        printf("Error: Could not open output.ppm for writing.\n");
    }
}

float intersectPlane(Ray ray, Vector3 pointOnPlane, Vector3 planeNormal, float tMin, float tMax)
{
    float denom = dot(ray.direction, planeNormal);
    if (fabs(denom) < 1e-6)
        return tMax;

    float t = dot(pointOnPlane - ray.origin, planeNormal) / denom;
    if (t > tMin && t < tMax)
        return t;
    return tMax;
}

float intersectSphere(Ray ray, Vector3 center, float radius, float tMin, float tMax)
{
    Vector3 oc = ray.origin - center;

    float A = dot(ray.direction, ray.direction);
    float B = 2.0f * dot(oc, ray.direction);
    float C = dot(oc, oc) - radius * radius;

    float D = B * B - 4 * A * C;
    if (D < 0)
        return tMax;

    float sqrtD = sqrt(D);
    float t0 = (-B - sqrtD) / (2 * A);
    float t1 = (-B + sqrtD) / (2 * A);

    if (t0 > tMin && t0 < tMax) return t0;
    if (t1 > tMin && t1 < tMax) return t1;

    return tMax;
}

std::string renderFrameToString(std::uint16_t width, std::uint16_t height, const std::vector<Vector3>& data)
{
    std::stringstream buffer;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const Vector3& pixel = data[y * width + x];
            int r = (int)std::max(0.0f, std::min(pixel.x() * 255.999f, 255.0f));
            int g = (int)std::max(0.0f, std::min(pixel.y() * 255.999f, 255.0f));
            int b = (int)std::max(0.0f, std::min(pixel.z() * 255.999f, 255.0f));

            // \x1B[48;2;R;G;Bm : Установка TrueColor (24-bit) фона
            buffer << "\x1B[48;2;" << r << ";" << g << ";" << b << "m ";
            // \x1B[0m : Сброс цвета
            buffer << "\x1B[0m";
        }
        buffer << "\n";
    }

    return buffer.str();
}

void resetConsoleCursor()
{
    printf("\x1B[H");
}

void App::updateCameraVectors()
{
    float yawRad = cameraYaw_;
    float pitchRad = cameraPitch_;

    // Ограничение тангажа (pitch) для предотвращения "переворота" камеры
    // Углы должны быть в диапазоне (-ПИ/2, ПИ/2) или (-89.9, 89.9) градусов
    if (pitchRad > 1.55f) pitchRad = 1.55f;
    if (pitchRad < -1.55f) pitchRad = -1.55f;

    // Вычисляем forward-вектор (cameraDirection_)
    cameraForward_ = Vector3(cosf(yawRad) * cosf(pitchRad), sinf(pitchRad), sinf(yawRad) * cosf(pitchRad));
    cameraForward_ = unit_vector(cameraForward_);

    // Мировой "вверх" вектор (ось Y)
    Vector3 worldUp(0.0f, 1.0f, 0.0f);

    // Вычисляем cameraRight_
    cameraRight_ = unit_vector(cross(cameraForward_, worldUp));

    // Вычисляем cameraUp_
    cameraUp_ = unit_vector(cross(cameraRight_, cameraForward_));
}

App::App(std::uint16_t width, std::uint16_t height) : width_(width), height_(height)
{
    data_.resize(width_ * height_);

    // Начальная позиция камеры: немного поднята над полом Y=0.5
    cameraOrigin_ = Vector3(0.0f, 0.5f, 0.0f);

    // Начальные углы: смотрим прямо вперед
    cameraYaw_ = 1.570796f; // PI/2 (смотрит по Z) - если Z-вперед
    cameraPitch_ = 0.0f;

    updateCameraVectors();
}

bool App::isRun() const
{
    return isRun_;
}

void App::update()
{

    const float aspectRatio = float(width_) / height_;

    // Вычисляем параметры плоскости проекции (как сенсор камеры)
    float viewportHeight = 2.0f; // Условная высота вьюпорта
    float viewportWidth = viewportHeight * aspectRatio; // Ширина с учетом аспекта экрана

    // Применяем компенсацию аспекта консольного символа к вертикали
    viewportHeight *= CONSOLE_CHAR_ASPECT_RATIO;

    // Расстояние до плоскости проекции (FOCAL_LENGTH = 1.0f)
    const Vector3 viewportCenter = cameraOrigin_ + cameraForward_ * FOCAL_LENGTH;

    // Вектор от центра вьюпорта к левому верхнему углу
    const Vector3 leftTop = viewportCenter - cameraRight_ * (viewportWidth / 2.0f) + cameraUp_ * (viewportHeight / 2.0f);

    for (int y = 0; y < height_; ++y)
    {
        for (int x = 0; x < width_; ++x)
        {
            // u, v - нормализованные координаты на экране (от 0 до 1)
            const float u = float(x) / width_;
            const float v = float(y) / height_;

            // Вычисляем точку на плоскости проекции
            const Vector3 pixPos = leftTop + cameraRight_ * (u * viewportWidth) - cameraUp_ * (v * viewportHeight);

            // Направление луча: от камеры через пиксель
            const Ray ray = { cameraOrigin_, unit_vector(pixPos - cameraOrigin_) };

            data_[y * width_ + x] = Vector3( ray.direction.x() * 0.5f + 0.5f, ray.direction.y() * 0.5f + 0.5f, ray.direction.z() * 0.5f + 0.5f);

            float tMin = 0.001f;
            float tMax = 10000.0f;

            // Плоскость (Пол)
            Vector3 planeNormal(0, 1, 0);
            Vector3 planePoint(0, -1, 0);

            float t = intersectPlane(ray, planePoint, planeNormal, tMin, tMax);
            if (t < tMax)
            {
                Vector3 pos = ray.origin + ray.direction * t;
                if ((int(pos.x() + 1000) % 2) != (int(pos.z() + 1000) % 2))
                    data_[y * width_ + x] = Vector3(1, 0.5, 0.5);
                else
                    data_[y * width_ + x] = Vector3(0.5, 0.5, 0.5);

                tMax = t;
            }

            // Сфера
            t = intersectSphere(ray, Vector3(0.0f, -0.5f, 7.0f), 1.5f, tMin, tMax);
            if (t < tMax)
            {
                data_[y * width_ + x] = Vector3(0.5f, 0.9f, 0.5f);
            }
        }
    }

    std::string frameBuffer = renderFrameToString(width_, height_, data_);
    resetConsoleCursor();
    std::cout << frameBuffer << std::flush;
}

void App::save() const
{
    saveImageToFile(width_, height_, data_);
}

bool App::input()
{
    if (_kbhit())
    {
        int key = _getch();
        
        float moveSpeed = 0.5f;
        float rotSpeed = 0.1f;

        if (key == 0 || key == 224)
        {
            int arrow = _getch();

            if (arrow == 72) cameraPitch_ += rotSpeed;  // ↑
            else if (arrow == 80) cameraPitch_ -= rotSpeed;  // ↓
            else if (arrow == 75) cameraYaw_ -= rotSpeed;  // ←
            else if (arrow == 77) cameraYaw_ += rotSpeed;  // →

            if (arrow == 72 || arrow == 80 || arrow == 75 || arrow == 77)
            {
                updateCameraVectors();
                return true;
            }

            return true;
        }

        switch (key)
        {
        case 'w': cameraOrigin_ += cameraForward_ * moveSpeed; break;
        case 's': cameraOrigin_ -= cameraForward_ * moveSpeed; break;
        case 'a': cameraOrigin_ -= cameraRight_ * moveSpeed; break;
        case 'd': cameraOrigin_ += cameraRight_ * moveSpeed; break;
        case 'q': cameraOrigin_ += cameraUp_ * moveSpeed; break;
        case 'e': cameraOrigin_ -= cameraUp_ * moveSpeed; break;
        case 27: isRun_ = false; break; // ESC
        }
        return true;
    }
    return false;
}