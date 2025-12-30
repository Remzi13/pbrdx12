#pragma once 

#include "core/std_types.h"

#include "render/common.h"
#include "render/sync_point.h"

#include <mutex>


namespace render {

	class Device;
	class ResourceView;
	class CommandContext;

	struct FreeList
	{
	public:
		FreeList(uint32 size);
		~FreeList();

		uint32 allocate();
		void free(uint32 index);

		bool canAllocate() const;

	private:
		vector<uint32> list_;
		uint32 count_ = 0;
	};

	class CPUDescriptorHeap : public DeviceObject
	{
	public:
		CPUDescriptorHeap(Device* pParent, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptors);

		CD3DX12_CPU_DESCRIPTOR_HANDLE allocateDescriptor();
		void freeDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
		FreeList freeList_;
		uint32 numDescriptors_;
		uint32 descriptorSize_ = 0;
		std::mutex mutex_;
	};



	class DescriptorHandle
	{
	public:
		DescriptorHandle();

		DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, uint32 heapIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = InvalidGPUHandle);

		[[nodiscard]] DescriptorHandle offset(uint32 numDescriptors, uint32 descriptorSize) const;

		void offsetInline(uint32 numDescriptors, uint32 descriptorSize);

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
		{
		}
		DescriptorHandle StartHandle;
		uint32 Size;
		uint32 CurrentOffset;
		SyncPoint SyncPoint;
	};

	class GPUDescriptorHeap : public DeviceObject
	{
	public:
		GPUDescriptorHeap(Device* pParent, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 dynamicPageSize, uint32 numDescriptors);
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

		FreeList persistentHandles_;
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

		void bindStagedDescriptors(CommandContext* context);

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
		std::array<StagedDescriptorTable, MAX_ROOT_SIGNATURE_PARAM> stagedDescriptors_{};
		vector<uint32> staleRootParameters_{};
		//BitField<RootSignature::sMaxNumParameters, uint8> m_StaleRootParameters{};
		//
		GPUDescriptorHeap* heapAllocator_{ nullptr };
		DescriptorHeapPage* currentHeapPage_{ nullptr };
		vector<DescriptorHeapPage*> releasedPages_;
	};

}