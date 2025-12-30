#pragma once 

#include "render/common.h"


namespace render {

	using namespace memory;

	class Texture;
	class Fence;

	class SwapChain : public DeviceObject
	{
	public:
		SwapChain(Device* device, HWND hwnd, int width, int height, int numFrames, ResourceFormat format);

		void resize(int width, int height);
		[[nodiscard]] Texture* backBuffer(int index) const;
		[[nodiscard]] int currentBackbuffer() const;
		void present();

	protected:
		int numFrames_;

		IDXGISwapChain* swapChain_;
		ResourceFormat format_;
		vector<UniquePtr<Texture>> backBuffers_;
		int currentBackBuffer_{ 0 };
		uint64 currentFence_{ 0 };

		UniquePtr<Fence> nFence_;
		HWND hwnd_;
	};
}