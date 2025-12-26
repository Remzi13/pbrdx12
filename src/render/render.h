#pragma once
#define NOMINMAX
#include <windows.h> 

#include "core/memory.h"

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
		SharedPtr<class Device> device_;
	};
}