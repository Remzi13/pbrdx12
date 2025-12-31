#include "render/device/dx12/descriptorHeap.h"

#include "render/device/dx12/device.h"
#include "render/device/dx12/command_context.h"


namespace render {

	CPUDescriptorHeap::CPUDescriptorHeap(GraphicsDevice* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptors)
		: DeviceObject(device), freeList_(numDescriptors), numDescriptors_(numDescriptors)//, m_Type(type)
	{
		auto* rawDevice = dynamic_cast<DeviceDx12*>(device);

		descriptorSize_ = rawDevice->device()->GetDescriptorHandleIncrementSize(type);

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;
		desc.NumDescriptors = numDescriptors;
		desc.Type = type;

		rawDevice->device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_));
		SetObjectName(heap_.Get(), "Offline Descriptor Heap");
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHeap::allocateDescriptor()
	{
		std::lock_guard lock(mutex_);

		return CD3DX12_CPU_DESCRIPTOR_HANDLE(heap_->GetCPUDescriptorHandleForHeapStart(), freeList_.allocate(),
		                                     descriptorSize_);
	}

	void CPUDescriptorHeap::freeDescriptor( D3D12_CPU_DESCRIPTOR_HANDLE descriptor )
	{
		std::lock_guard lock( mutex_ );
		uint32 elementIndex = static_cast<uint32>((descriptor.ptr - heap_->GetCPUDescriptorHandleForHeapStart().ptr) / descriptorSize_);
		freeList_.free( elementIndex );
	}

	GPUDescriptorHeap::GPUDescriptorHeap(GraphicsDevice* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 dynamicPageSize, uint32 numDescriptors)
		: DeviceObject(device), dynamicPageSize_(dynamicPageSize), numDescriptors_(numDescriptors), numDynamicDescriptors_(numDescriptors / 2), 
		numPersistentDescriptors_(numDescriptors / 2), persistentHandles_(numDescriptors / 2), type_(type)
	{
		ASSERT(dynamicPageSize >= 32, "Page size must be at least 128 (is %d)", dynamicPageSize);
		ASSERT(numDynamicDescriptors_ % dynamicPageSize == 0, "Number of descriptors must be a multiple of Page Size (%d)", dynamicPageSize);
		ASSERT(type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, "Online Descriptor Heap must be either of CBV/SRV/UAV or Sampler type.");

		auto* rawDevice = dynamic_cast<DeviceDx12*>(device);

		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NodeMask = 0;
		desc.NumDescriptors = numDescriptors;
		desc.Type = type;
		rawDevice->device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(heap_.GetAddressOf()));
		SetObjectName(heap_.Get(), type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? "GPU CBV/SRV/UAV Descriptor Heap" : "GPU Sampler Descriptor Heap");
		
		descriptorSize_ = rawDevice->device()->GetDescriptorHandleIncrementSize(type);
		startHandle_  = DescriptorHandle(heap_->GetCPUDescriptorHandleForHeapStart(), 0, heap_->GetGPUDescriptorHandleForHeapStart());
		
		uint32 numPages = numDynamicDescriptors_ / dynamicPageSize;
		
		DescriptorHandle currentOffset = startHandle_.Offset(numPersistentDescriptors_, descriptorSize_);
		for (uint32 i = 0; i < numPages; ++i)
		{
			dynamicPages_.emplace_back(makeUnique<DescriptorHeapPage>(currentOffset, dynamicPageSize));
			freeDynamicPages_.push_back(dynamicPages_.back().get());
			currentOffset.OffsetInline(dynamicPageSize, descriptorSize_);
		}
	}

	GPUDescriptorHeap::~GPUDescriptorHeap()
	{
		cleanupPersistent();
		cleanupDynamic();
	}


	DescriptorHeapPage* GPUDescriptorHeap::allocateDynamicPage()
	{
		//std::lock_guard lock(m_DynamicPageAllocateMutex);
		if (freeDynamicPages_.empty())
		{
			cleanupDynamic();
		}

		ASSERT(!freeDynamicPages_.empty(), "Ran out of dynamic descriptor heap space (%d). Increase heap size.", numDynamicDescriptors_);
		DescriptorHeapPage* pPage = freeDynamicPages_.back();
		freeDynamicPages_.pop_back();
		return pPage;
	}

	void GPUDescriptorHeap::freeDynamicPage(const SyncPoint& syncPoint, DescriptorHeapPage* pPage)
	{
		//std::lock_guard lock(m_DynamicPageAllocateMutex);
		pPage->SyncPoint = syncPoint;
		pPage->CurrentOffset = 0;
		releasedDynamicPages_.push(pPage);
	}
	
	void GPUDescriptorHeap::cleanupDynamic()
	{
		while (!releasedDynamicPages_.empty())
		{
			DescriptorHeapPage* pPage = releasedDynamicPages_.front();
			if (!pPage->SyncPoint.isComplete())
				break;

			releasedDynamicPages_.pop();
			freeDynamicPages_.push_back(pPage);
		}
	}

	DescriptorHandle GPUDescriptorHeap::allocatePersistent()
	{
		std::lock_guard lock(allocationLock_);
		if (!persistentHandles_.canAllocate())
		{
			cleanupPersistent();
		}

		ASSERT(persistentHandles_.canAllocate(), "Out of persistent descriptor heap space (%d), increase heap size", numPersistentDescriptors_);
		return startHandle_.Offset(persistentHandles_.allocate(), descriptorSize_);
	}

	void GPUDescriptorHeap::cleanupPersistent()
	{
		while (!persistentDeletionQueue_.empty())
		{
			const auto&[fst, snd] = persistentDeletionQueue_.front();
			// TODO : global fence
			//if (!GetParent()->GetFrameFence()->IsComplete(f.second))
			//	break;

			persistentHandles_.free(fst);
			persistentDeletionQueue_.pop();
		}
	}

	GPUDescriptorAllocator::GPUDescriptorAllocator(GPUDescriptorHeap* pGlobalHeap) 
		: DeviceObject(pGlobalHeap->parent()), heapAllocator_(pGlobalHeap)
	{
	}

	GPUDescriptorAllocator::~GPUDescriptorAllocator()
	{
		if (currentHeapPage_)
		{
			releasedPages_.push_back(currentHeapPage_);
		}
		// TODO : global fence 
		// Fence* pFrameFence = GetParent()->GetFrameFence();
		//SyncPoint syncPoint(pFrameFence, pFrameFence->GetLastSignaledValue());
		//ReleaseUsedHeaps(syncPoint);
	}

	DescriptorHandle GPUDescriptorAllocator::allocate(uint32 count)
	{
		if (!currentHeapPage_ || currentHeapPage_->Size - currentHeapPage_->CurrentOffset < count)
		{
			if (currentHeapPage_)
				releasedPages_.push_back(currentHeapPage_);
			currentHeapPage_ = heapAllocator_->allocateDynamicPage();
		}

		DescriptorHandle handle = currentHeapPage_->StartHandle.Offset(currentHeapPage_->CurrentOffset, heapAllocator_->descriptorSize());
		currentHeapPage_->CurrentOffset += count;
		return handle;
	}

	void GPUDescriptorAllocator::bindStagedDescriptors(CommandContext& context)
	{
		for (uint32 rootIndex : staleRootParameters_)
		{
			StagedDescriptorTable& table = stagedDescriptors_[rootIndex];
			DescriptorHandle handle = allocate(static_cast<uint32>(table.Descriptors.size()));
			for (int i = table.StartIndex; i < table.Descriptors.size(); ++i)
			{
				if (table.Descriptors[i].ptr != DescriptorHandle::InvalidCPUHandle.ptr && table.Descriptors[i].ptr != 0)
				{
					DescriptorHandle target = handle.Offset(i, heapAllocator_->descriptorSize());
					static_cast<DeviceDx12*>(parent())->device()->CopyDescriptorsSimple(1, target.CpuHandle, table.Descriptors[i], heapAllocator_->type());
				}
			}
			table.Descriptors.clear();
			table.StartIndex = 0xFFFFFFFF;
		
		//	if (descriptorTableType == CommandListContext::Graphics)
		//		context.GetCommandList()->SetGraphicsRootDescriptorTable(rootIndex, handle.GpuHandle);
		//	else if (descriptorTableType == CommandListContext::Compute)
		//		context.GetCommandList()->SetComputeRootDescriptorTable(rootIndex, handle.GpuHandle);
		//	else
		//		gUnreachable();
			dynamic_cast<CommandContextDx12*>(&context)->list()->SetGraphicsRootDescriptorTable(rootIndex, handle.GpuHandle);
		}
		staleRootParameters_.clear();
	}
	void GPUDescriptorAllocator::setDescriptors(uint32 rootIndex, uint32 offset, const ResourceView* handle)
	{

		//m_StaleRootParameters.SetBit(rootIndex);
		staleRootParameters_.push_back(rootIndex);
		StagedDescriptorTable& table = stagedDescriptors_[rootIndex];
		//gAssert(table.Capacity != 0, "Root parameter at index '%d' is not a descriptor table", rootIndex);
		//gAssert(offset + handles.GetSize() <= table.Capacity, "Descriptor table at root index '%d' is too small (is %d but requires %d)", rootIndex, table.Capacity, offset + handles.GetSize());

		table.Descriptors.resize(core::Max((uint32)table.Descriptors.size(), uint32(1)));
		table.StartIndex = core::Min(offset, table.StartIndex);
		table.Descriptors[offset] = handle->descriptor();
	}

	void GPUDescriptorAllocator::releaseUsedHeaps(const SyncPoint& syncPoint)
	{
		for (DescriptorHeapPage* pPage : releasedPages_)
			heapAllocator_->freeDynamicPage(syncPoint, pPage);
		releasedPages_.clear();
	}

}