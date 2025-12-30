#include "render.h"

#define NOMINMAX
#include <windows.h> 

#include "render/device.h"
#include "render/swapchain.h"

namespace render {

	bool Render::init(HWND hwnd)
	{
		device_ = makeShared<Device>();

		device_->init();

		swapChain_ = makeUnique<SwapChain>(device_.get(), hwnd, 800, 600, 2, format_);

		viewport_ = device_->createTexture(TextureDesc::create2D(800, 600, format_, Colors::Green, TextureFlag::ShaderResource | TextureFlag::RenderTarget), "Viewport");


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
		swapChain_->present();

		device_->tick();
	}
}