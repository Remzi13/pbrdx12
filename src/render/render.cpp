#include "render.h"

#define NOMINMAX
#include <windows.h> 

#include "render/device.h"

namespace render {

	bool Render::init(HWND hwnd)
	{
		device_ = makeShared<Device>();

		device_->init();

		return false;
	}

	void Render::fini()
	{

	}

	void Render::update(float dt)
	{
	}

	void Render::draw()
	{
	}
}