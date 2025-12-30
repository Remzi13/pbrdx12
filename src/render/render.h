#pragma once
#define NOMINMAX
#include <windows.h> 

#include "core/memory.h"

#include "render/common.h"

namespace render {
	
	using namespace memory;

	class Render
	{
	public:
		~Render() = default;

		bool init(HWND hwnd);
		void fini();

		void update(float dt);
		void draw();

	private:
		ResourceFormat format_{ ResourceFormat::RGBA8_UNORM };
		SharedPtr<class Device> device_;
		SharedPtr<class SwapChain> swapChain_;
		SharedPtr<class Texture> viewport_;
	};
}