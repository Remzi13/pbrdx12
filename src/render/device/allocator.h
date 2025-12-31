#pragma once

#include "core/memory.h"

namespace elm::render {
	class Buffer;
}

namespace elm::render::device {
	using namespace memory;
	struct Allocation
	{
		SharedPtr<Buffer> resource;
		uint64 Location = 0;
		uint64 Offset = 0;
		uint64 Size = 0;
		void* mappedMemory = nullptr;
	};

}