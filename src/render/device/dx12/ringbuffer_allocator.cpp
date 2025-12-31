#include "render/device/dx12/ringbuffer_allocator.h"

#include "render/device/dx12/device.h"
#include "render/device/dx12/resource.h"

namespace render::device {

	RingBufferAllocator::RingBufferAllocator(GraphicsDevice* parent, uint32 size) 
		: DeviceObject(parent), size_(size)
	{
		buffer_ = static_cast<DeviceDx12*>(parent)->createBuffer(Buffer::Desc{.Size = size, .Flags = BufferFlag::Upload}, nullptr, 0, "RingBuffer");
	}

	RingBufferAllocator::~RingBufferAllocator()
	{
		sync();
		size_ = 0;
		consumeOffset_ = 0;
		produceOffset_ =0;
	}

	bool RingBufferAllocator::allocate(uint32 size, Allocation& allocation)
	{
		// TODO : multithreading
	//	std::lock_guard lock(m_Lock);
		while (!retiredAllocations_.empty())
		{
			const RetiredAllocation& retired = retiredAllocations_.front();
			if (!retired.Sync.isComplete())
				break;
			consumeOffset_ = retired.Offset + retired.Size;
			retiredAllocations_.pop();
		}

		constexpr uint32 InvalidOffset = 0xFFFFFFFF;
		uint32 offset = InvalidOffset;

		if (size > size_)
			return false;

		if (produceOffset_ >= consumeOffset_)
		{
			if (produceOffset_ + size <= size_)
			{
				offset = produceOffset_;
				produceOffset_ += size;
			}
			else if (size <= consumeOffset_)
			{
				offset = 0;
				produceOffset_ = size;
			}
		}
		else if (produceOffset_ + size <= consumeOffset_)
		{
			offset = produceOffset_;
			produceOffset_ += size;
		}

		if (offset == InvalidOffset)
			return false;

		allocation.context_ = parent()->getCommandContext(CommandQueue::COPY);
		allocation.Offset = offset;
		allocation.Size = size;
		allocation.GpuHandle = static_cast<BufferDx12*>(buffer_.get())->resource()->GetGPUVirtualAddress() + offset;
		allocation.resource_ = buffer_;
		allocation.mappedMemory = (char*)buffer_->mappedData() + offset;

		return true;
	}

	void RingBufferAllocator::free(Allocation& allocation)
	{
		// TODO : multithreading
		//std::lock_guard lock(m_Lock);

		RetiredAllocation retired;
		retired.Offset = allocation.Offset;
		retired.Size = allocation.Size;
		retired.Sync = allocation.context_->execute();
		retiredAllocations_.push(retired);

		allocation.resource_ = nullptr;
		allocation.context_ = nullptr;
		allocation.mappedMemory= nullptr;

		lastSync_ = retired.Sync;
	}

	void RingBufferAllocator::sync()
	{
		if (lastSync_.isValid())
			lastSync_.wait();
	}
}