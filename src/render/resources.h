#pragma once

#include "render/common.h"

#include "render/descriptor_heap.h"

namespace render {

	using namespace memory;

	class DeviceResource : public DeviceObject
	{
	public:
		DeviceResource(Device* device, ID3D12ResourceX* resource);
		~DeviceResource();

		void setName(const char* name);
		void setNeedStateTracking(bool needsTraking) { needsStateTracking_ = needsTraking; }
		void setImmediateDelete() { immediateDelete_ = true; }

		void setResourceState(ResourceState state) { state_ = state; }
		ResourceState resourceState() const { return state_; }

		ID3D12ResourceX* resource() const { return resource_; }
		D3D12_GPU_VIRTUAL_ADDRESS gpuHandle() const { return resource_->GetGPUVirtualAddress(); }

	private:
		string name_;
		ID3D12ResourceX* resource_{ nullptr };
		bool needsStateTracking_{ false };
		bool immediateDelete_{ false };
		ResourceState state_{ ResourceState::Unknown };
	};


	class ResourceView : public DeviceObject
	{
	public:
		ResourceView(DeviceResource* resource, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor, DescriptorHandle gpuDescriptor)
			: DeviceObject(resource->parent()), resource_(resource), cpuDescriptor_(cpuDescriptor), gpuDescriptor_(gpuDescriptor)
		{
		};
		D3D12_CPU_DESCRIPTOR_HANDLE descriptor() const { return cpuDescriptor_; }
		uint32 heapIndex() const { return gpuDescriptor_.HeapIndex; }
	private:
		DeviceResource* resource_ = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor_ = {};
		DescriptorHandle gpuDescriptor_;
	};

	class ShaderResourceView : public ResourceView
	{
	public:
		ShaderResourceView(DeviceResource* parent, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor, DescriptorHandle gpuDescriptor)
			: ResourceView(parent, cpuDescriptor, gpuDescriptor)
		{
		}
	};

	enum class BufferFlag : uint8
	{
		None = 0,
		UnorderedAccess = 1 << 0,
		ShaderResource = 1 << 1,
		Upload = 1 << 2,
		Readback = 1 << 3,
		ByteAddress = 1 << 4,
		AccelerationStructure = 1 << 5,
		IndirectArguments = 1 << 6,
		NoBindless = 1 << 7,
	};

	inline constexpr BufferFlag operator| (BufferFlag  Lhs, BufferFlag Rhs)
	{
		// Cast to underlying type to avoid enum class promotion
		return (BufferFlag)((__underlying_type(BufferFlag))Lhs | (__underlying_type(BufferFlag))Rhs);
	}


	class Buffer : public DeviceResource
	{
	public:
		struct Desc
		{
			uint64			Size = 0;
			uint32			ElementSize = 1;
			BufferFlag		Flags = BufferFlag::None;
			ResourceFormat	Format = ResourceFormat::Unknown;

			uint32 numElements() const { return static_cast<uint32>(Size / ElementSize); }
		};

		struct VertexView
		{
			//D3D12_GPU_VIRTUAL_ADDRESS Location;
			uint64 Location;
			uint32 Elements;
			uint32 Stride;
			uint32 OffsetFromStart;
		};

		struct IndexView
		{
			//D3D12_GPU_VIRTUAL_ADDRESS Location;
			uint64 Location;
			uint32 Elements;
			uint32 OffsetFromStart;
			ResourceFormat Format;
		};

	public:
		Buffer(Device* device, const Buffer::Desc& desc, ID3D12ResourceX* resource)
			: DeviceResource(device, resource), desc_(desc)
		{
		}
		void setResourceState(D3D12_RESOURCE_STATES state, uint32 subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) { state_ = state; }

		void setSRV(const SharedPtr<ShaderResourceView>& srv) { srv_ = srv; }
		ShaderResourceView* srv() const { return srv_.get(); }

		void* mappedData() const { return mappedData_; }
		void setMappedData(void* data) { mappedData_ = data; }

		uint64 size() const { return desc_.Size; }

		const Desc& desc() const { return desc_; }

	private:
		Desc desc_;
		void* mappedData_{ nullptr };
		D3D12_RESOURCE_STATES state_;
		SharedPtr<ShaderResourceView> srv_;
	};

	struct BufferSRVDesc
	{
		BufferSRVDesc(ResourceFormat format = ResourceFormat::Unknown, bool raw = false, uint32 elementOffset = 0, uint32 numElements = 0)
			: Format(format), Raw(raw), ElementOffset(elementOffset), NumElements(numElements)
		{
		}

		ResourceFormat Format;
		bool Raw;
		uint32 ElementOffset;
		uint32 NumElements;
	};
}

