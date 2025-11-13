//
// Created by zakkh on 10.11.2025.
//

#ifndef RTDX12_APP_H
#define RTDX12_APP_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "render.h"
#include "input.h"

#include <vector>

class App {
public:

	bool init(HWND hwnd, std::uint16_t width, std::uint16_t height);
	void update();
	void fini();

private:
	void inputUpdate();

	void handleKeyEvent( const InputEvent& event );

private:
	Camera camera_;
	Render render_;

	std::vector<Primitive> scene_;
	bool isDirty_{ true };
};

#endif // RTDX12_APP_H
