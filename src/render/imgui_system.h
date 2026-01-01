#pragma once

#include "render/render.h"

using WindowHandle = HWND;

namespace render::ImGuiSystem {

	void init(render::Render* render, WindowHandle window);
	bool input(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void update(float dt);
	void render(render::Render* render);

}