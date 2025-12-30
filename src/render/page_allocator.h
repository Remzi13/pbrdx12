#pragma once

#include "render/resources.h"

namespace render {
	using namespace memory;

	class Buffer;

	struct Allocation
	{
		SharedPtr<Buffer> resource;
		uint64 Location = 0;
		uint64 Offset = 0;
		uint64 Size = 0;
		void* mappedMemory = nullptr;
	};

	class PageAllocator : public DeviceObject
	{
	public:
		PageAllocator(Device* pParent, BufferFlag bufferFlags, uint64 pageSize);

		SharedPtr<Buffer> allocate();
		void free(const SyncPoint& syncPoint, const vector<SharedPtr<Buffer>>& pPages);
		uint64 pageSize() const { return pageSize_; }

	private:
		BufferFlag bufferFlags_;
		uint64 pageSize_;
		queue<std::pair<SharedPtr<Buffer>, SyncPoint>> pagesPool_;
		//FencedPool<Ref<Buffer>, true> m_PagePool;
	};

	class Allocator
	{
	public:
		void init(PageAllocator* pageAllocator);
		Allocation allocate(uint64 size, int aligment);
		void free(const SyncPoint& syncPoint);

	private:
		PageAllocator* pageAllocator_{ nullptr };
		SharedPtr<Buffer> currentPage_;
		vector<SharedPtr<Buffer>> usedPages_;
		uint64 currentOffset_{ 0 };
	};

}