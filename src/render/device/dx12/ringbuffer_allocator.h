#pragma once

#include "core/std_types.h"

#include "render/device/device_interface.h"

namespace render {
	namespace device {

		class RingBufferAllocator : public DeviceObject
		{
		public:
			struct Allocation
			{
				CommandContext* context_;
				SharedPtr<Buffer> resource_;
				D3D12_GPU_VIRTUAL_ADDRESS GpuHandle{ 0 };
				uint32 Offset = 0;
				uint32 Size = 0;
				void* mappedMemory{ nullptr };
			};
			
			RingBufferAllocator(GraphicsDevice* parent, uint32 size);
			~RingBufferAllocator();

			bool allocate(uint32 size, Allocation& allocation);
			void free(Allocation& allocation);
			void sync();

		private:
			struct RetiredAllocation
			{
				SyncPoint Sync;
				uint32 Offset;
				uint32 Size;
			};
			queue<RetiredAllocation> retiredAllocations_;

			uint32 size_;
			uint32 consumeOffset_;
			uint32 produceOffset_;

			SyncPoint lastSync_;
			SharedPtr<Buffer> buffer_;
		};
	}
}