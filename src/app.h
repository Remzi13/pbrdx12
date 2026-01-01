//
// Created by zakkh on 10.11.2025.
//

#ifndef RTDX12_APP_H
#define RTDX12_APP_H

#define NOMINMAX
#include <windows.h>

#include "input.h"
#include "scene.h"

#include "render/render.h"

#include <vector>

class App {
public:

	bool init(HWND hwnd );
	void update();
	void fini();

	bool input(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	void inputUpdate();

	void handleKeyEvent( const InputEvent& event );

private:
	Scene scene_;
	bool isDirty_{ true };
	render::Render render_;
	HWND hwnd_;
};

#endif // RTDX12_APP_H
