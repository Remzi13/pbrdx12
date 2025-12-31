#pragma once

#include "render/device/device_interface.h"

#include "D3D12/include/d3d12.h"
#include "D3D12/include/d3dx12/d3dx12.h"

#include "data/custom_structs.h"

#include <mutex>

namespace elm::render {

	class DescriptorHandle
	{
	public:
		DescriptorHandle()
			: CpuHandle(InvalidCPUHandle), GpuHandle(InvalidGPUHandle), HeapIndex(InvalidHeapIndex)
		{}

		DescriptorHandle( D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, uint32 heapIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = InvalidGPUHandle)
			: CpuHandle(cpuHandle), GpuHandle(gpuHandle), HeapIndex(heapIndex)
		{}

		[[nodiscard]] DescriptorHandle Offset(uint32 numDescriptors, uint32 descriptorSize) const {
			DescriptorHandle handle = *this;
			handle.OffsetInline(numDescriptors, descriptorSize);
			return handle;
		}

		void OffsetInline(uint32 numDescriptors, uint32 descriptorSize)
		{
			if (CpuHandle != InvalidCPUHandle)
				CpuHandle.Offset(numDescriptors, descriptorSize);
			if (GpuHandle != InvalidGPUHandle)
				GpuHandle.Offset(numDescriptors, descriptorSize);
			if (HeapIndex != InvalidHeapIndex)
				HeapIndex += numDescriptors;
		}


		constexpr static D3D12_CPU_DESCRIPTOR_HANDLE InvalidCPUHandle = { ~0u };
		constexpr static D3D12_GPU_DESCRIPTOR_HANDLE InvalidGPUHandle = { ~0u };
		constexpr static uint32 InvalidHeapIndex = 0xFFFFFFFF;

		CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE GpuHandle;
		uint32 HeapIndex;
	};
	


	struct DescriptorHeapPage
	{
		DescriptorHeapPage(const DescriptorHandle& startHandle, uint32 size)
			: StartHandle(startHandle), Size(size), CurrentOffset(0)
		{}
		DescriptorHandle StartHandle;
		uint32 Size;
		uint32 CurrentOffset;
		SyncPoint SyncPoint;
	};

	class CPUDescriptorHeap : public DeviceObject
	{
	public:
		CPUDescriptorHeap(GraphicsDevice* pParent, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptors);

		CD3DX12_CPU_DESCRIPTOR_HANDLE allocateDescriptor();
		void freeDescriptor( D3D12_CPU_DESCRIPTOR_HANDLE descriptor );

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
		core::FreeList freeList_;
		uint32 numDescriptors_;
		uint32 descriptorSize_ = 0;
		std::mutex mutex_;
	};

	class GPUDescriptorHeap : public DeviceObject
	{
	public:
		GPUDescriptorHeap(GraphicsDevice* pParent, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 dynamicPageSize, uint32 numDescriptors);
		~GPUDescriptorHeap();

		uint32 descriptorSize() const { return descriptorSize_; }

		DescriptorHeapPage* allocateDynamicPage();
		void freeDynamicPage(const SyncPoint& syncPoint, DescriptorHeapPage* pPage);

		DescriptorHandle allocatePersistent();
		ID3D12DescriptorHeap* heap() const { return heap_.Get(); }
		D3D12_DESCRIPTOR_HEAP_TYPE type() const { return type_; }

	private:
		void cleanupPersistent();
		void cleanupDynamic();

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
		D3D12_DESCRIPTOR_HEAP_TYPE type_{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };

		uint32 dynamicPageSize_;
		uint32 numDescriptors_;
		uint32 numDynamicDescriptors_;
		uint32 numPersistentDescriptors_;
		uint32 descriptorSize_ = 0;
		DescriptorHandle startHandle_;
		std::mutex allocationLock_;

		core::FreeList persistentHandles_;
		vector<std::unique_ptr<DescriptorHeapPage>> dynamicPages_;
		queue<DescriptorHeapPage*> releasedDynamicPages_;
		vector<DescriptorHeapPage*> freeDynamicPages_;
		queue<std::pair<uint32, uint64>> persistentDeletionQueue_;
	};

	class GPUDescriptorAllocator : public DeviceObject
	{
	public:
		GPUDescriptorAllocator(GPUDescriptorHeap* pGlobalHeap);
		~GPUDescriptorAllocator();

		DescriptorHandle allocate(uint32 count);

		void setDescriptors(uint32 rootIndex, uint32 offset, const ResourceView* handles);

		void bindStagedDescriptors(CommandContext& context);

		//void ParseRootSignature(const RootSignature* pRootSignature);
		void releaseUsedHeaps(const SyncPoint& syncPoint);

	private:

		//// Structure holding staged descriptors for a table.
		struct StagedDescriptorTable
		{
			vector<D3D12_CPU_DESCRIPTOR_HANDLE> Descriptors;
			uint32 StartIndex = 0xFFFFFFFF;
			uint32 Capacity = 0;
		};
		array<StagedDescriptorTable, RootSignature::MaxNumParameters> stagedDescriptors_{};
		vector<uint32> staleRootParameters_{};
		//BitField<RootSignature::sMaxNumParameters, uint8> m_StaleRootParameters{};
		//
		GPUDescriptorHeap* heapAllocator_{ nullptr };
		DescriptorHeapPage* currentHeapPage_{ nullptr };
		vector<DescriptorHeapPage*> releasedPages_;
	};
}