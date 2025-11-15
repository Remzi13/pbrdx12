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
	bool isRun_{ true };
	std::uint16_t width_;
	std::uint16_t height_;
	Vector3 cameraOrigin_{ 0, 0, 0 };
	std::vector<Vector3> data_;
};