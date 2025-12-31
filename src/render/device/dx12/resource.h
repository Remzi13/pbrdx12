#pragma once

#include "core/std_types.h"

#include "render/device/dx12/descriptorHeap.h"

namespace render {

	class GraphicsDevice;

	class DeviceResource : public DeviceObject
	{
	public:
		DeviceResource( GraphicsDevice* device , ID3D12ResourceX* resource );
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
		{};
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
		{}
	};

	class BufferDx12 : public Buffer, public DeviceResource
	{
	public:
		BufferDx12(GraphicsDevice* device, const Buffer::Desc& desc, ID3D12ResourceX* resource)
			: DeviceResource(device, resource), Buffer(desc)
		{}
		void setResourceState(D3D12_RESOURCE_STATES state, uint32 subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) { state_ = state; }

		void setSRV(const SharedPtr<ShaderResourceView>& srv) { srv_ = srv; }
		ShaderResourceView* srv() const { return srv_.get(); }

	private:
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