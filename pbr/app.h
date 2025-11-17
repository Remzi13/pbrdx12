#pragma once

#include <cstdint>
#include <vector>

#include "../src/vector.h"



class App
{
public:
	App(std::uint16_t width, std::uint16_t height);

	bool isRun() const;
	void input();
	void update();

	void save() const;

private:
	void updateCameraVectors();

private:
	bool isRun_{ true };
	std::uint16_t width_;
	std::uint16_t height_;
	Vector3 cameraOrigin_{ 0, 0, 0 };
	std::vector<Vector3> data_;

	Vector3 cameraForward_ = Vector3(0, 0, 1);
	Vector3 cameraRight_ = Vector3(1, 0, 0);
	Vector3 cameraUp_ = Vector3(0, 1, 0);

	float cameraYaw_ = 0.0f;
	float cameraPitch_ = 0.0f;
};