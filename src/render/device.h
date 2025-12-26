#pragma once 

#include "core/memory.h"

#include "render/command_queue.h"

using Microsoft::WRL::ComPtr;

namespace render {

	using namespace memory;

	class Device
	{
	public: 
		bool init();		

		ID3D12Device* device() const;

	private:
		ComPtr<ID3D12Device> device_;

		UniquePtr<CommandQueue> graphicsQueue_;
	};
}