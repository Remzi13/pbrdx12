#pragma once

#include "render/device/device_interface.h"

#include "render/device/dx12/texture.h"
#include "render/device/dx12/fence.h"

#include <dxgi1_6.h>
namespace render {

	class DeviceDx12;

	class SwapChainDx12 : public SwapChain
	{
	public:
		SwapChainDx12(DeviceDx12* device, HWND hwnd, int width, int height, int numFrames, ResourceFormat format);

		void resize(int width, int height) override;
		Texture* backBuffer(int index) const override;
		int currentBackbuffer() const override;
		void present() override;

	private:
		IDXGISwapChain* swapChain_;
		ResourceFormat format_;
		vector<UniquePtr<TextureDx12>> backBuffers_;
		int currentBackBuffer_{ 0 };
		uint64 curretntFence_{ 0 };

		UniquePtr<device::Fence> nFence_;
		HWND hwnd_;
	};

}
