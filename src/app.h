//
// Created by zakkh on 10.11.2025.
//

#ifndef RTDX12_APP_H
#define RTDX12_APP_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "render.h"
#include "input.h"
#include "scene.h"

#include <vector>

class App {
public:

	bool init(HWND hwnd );
	void update();
	void fini();

private:
	void inputUpdate();

	void handleKeyEvent( const InputEvent& event );

private:
	Render render_;

	Scene scene_;
	bool isDirty_{ true };

	HWND hwnd_;
};

#endif // RTDX12_APP_H
