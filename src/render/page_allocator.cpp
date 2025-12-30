#include "render/page_allocator.h"

#include "core/debug.h"

#include "render/device.h"

#include <format>

namespace render {
	PageAllocator::PageAllocator(Device* device, BufferFlag bufferFlags, uint64 pageSize)
		: DeviceObject(device), bufferFlags_(bufferFlags), pageSize_(pageSize)
	{
	}

	SharedPtr<Buffer> PageAllocator::allocate()
	{
		if (pagesPool_.empty() || !pagesPool_.front().second.isComplete())
		{
			string name = std::format("Dynamic Allocation Buffer (%f KB)", memory::BytesToKiloBytes * pageSize_);
			auto buffer = parent()->createBuffer(Buffer::Desc{ .Size = pageSize_, .Flags = BufferFlag::Upload }, nullptr, 0, name.c_str(), nullptr);
			return buffer;
		}
		auto object = std::move(pagesPool_.front().first);
		pagesPool_.pop();
		return object;
	}

	void PageAllocator::free(const SyncPoint& syncPoint, const vector<SharedPtr<Buffer>>& pPages)
	{
		for (auto pPage : pPages)
		{
			pagesPool_.push({ std::move(pPage), syncPoint });
		}
	}

	void Allocator::init(PageAllocator* pageAllocator)
	{
		pageAllocator_ = pageAllocator;
	}

	Allocation Allocator::allocate(uint64 size, int alignment)
	{
		uint64 bufferSize = memory::alignUp<uint64>(size, alignment);
		Allocation allocation;
		allocation.Size = size;

		if (bufferSize > pageAllocator_->pageSize())
		{
			ASSERT(false);
			//	Ref<Buffer> pPage = m_pPageManager->GetParent()->CreateBuffer(BufferDesc{ .Size = size, .Flags = BufferFlag::Upload }, "Large Page");
			//	allocation.Offset = 0;
			//	allocation.GpuHandle = pPage->GetGpuHandle();
			//	allocation.pBackingResource = pPage;
			//	allocation.pMappedMemory = pPage->GetMappedData();
		}
		else
		{
			currentOffset_ = memory::alignUp<uint64>(currentOffset_, alignment);

			if (currentPage_ == nullptr || currentOffset_ + bufferSize >= currentPage_->size())
			{
				currentPage_ = pageAllocator_->allocate();
				currentOffset_ = 0;
				usedPages_.push_back(currentPage_);
			}
			allocation.Offset = currentOffset_;
			allocation.Location = currentPage_->gpuHandle() + currentOffset_;
			allocation.resource = currentPage_;
			allocation.mappedMemory = static_cast<char*>(currentPage_->mappedData()) + currentOffset_;

			currentOffset_ += bufferSize;
		}
		return allocation;
	}

	void Allocator::free(const SyncPoint& syncPoint)
	{
		pageAllocator_->free(syncPoint, usedPages_);
		usedPages_.clear();

		currentPage_ = nullptr;
		currentOffset_ = 0;
	}
}