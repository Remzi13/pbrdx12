#pragma once

#include "math/base_types.h"

#include "render/device/device_interface.h"
#include "render/utils.h"
#include "render/device/dx12/texture.h"
#include "render/device/dx12/page_allocator.h"


namespace elm::render {

	class DeviceDx12;

	class CommandContextDx12 : public CommandContext
	{
	public:

		CommandContextDx12(DeviceDx12* device, CommandQueue::Type type, GPUDescriptorHeap* descriptorHeap, device::PageAllocator* pageAllocator);
		
		void begin(RenderPassInfo info) override;
		void end() override;
		void reset() override;
		void flushResourceBarriers() override;
		void insertResourceBarrier(Texture* resource, ResourceState beforeState, ResourceState afterState, uint32 subResource) override;
		void copyBuffer(const Buffer* pSource, const Buffer* pTarget, uint64 size, uint64 sourceOffset, uint64 destinationOffset) override;
		void addResourceBarrier(Texture* pResource, ResourceState beforeState, ResourceState afterState, uint32 subResource) override;
		void setRootSignature(const RootSignature* rootSignature) override;		
		void setPipelineState(const PipelineState* pipelineState) override;
		void setPrimitiveTopology(const PrimitiveTopology topology) override;
		void setViewport(const math::RectF& rect, float minDepth = 0.0f, float maxDepth = 1.0f) override;
		device::Allocation allocate(uint64 size, uint32 alignment = 16u) override;
		void setVertexBuffer(Buffer::VertexView view) override;
		void setIndexBuffer(Buffer::IndexView view) override;
		void setScissorRect(const math::RectF& rect) override;
		SyncPoint execute() override;

		void bindRootCBV(uint32 rootIndex, const void* data, uint32 size) override;		
		void bindResources(uint32 rootIndex, const Buffer* pViews, uint32 offset = 0) override;

		void drawIndexedInstanced(uint32 indexCount, uint32 indexStart, uint32 instanceCount, uint32 minVertex = 0, uint32 instanceStart = 0) override;
		
		void free(SyncPoint syncPoint) override;

		ID3D12GraphicsCommandList* list() const;

	private:
		void addBarrier(const D3D12_RESOURCE_BARRIER& barrier);

		void prepareDraw();

		void clearState();

	private:
		struct PendingBarrier
		{
			DeviceResource* pResource;
			D3D12_RESOURCE_STATES State;
			uint32 Subresource;
		};

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_; 
		ID3D12DeviceX* deviceX_{ nullptr };
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
		unsigned int rtvDescriptorSize_;

		RenderPassInfo currentPass_;
		
		uint32 numBatchedBarriers_{ 0 };
		static constexpr uint32 MAX_BATCHED_BARRIERS = 64;
		array<D3D12_RESOURCE_BARRIER, MAX_BATCHED_BARRIERS> batchedBarriers_{};


		const RootSignature* currentRootSignature_{ nullptr };
		const PipelineState* currentPipelineState_{ nullptr };

		bool inRenderPass_{ false };

		device::Allocator allocator_;
		GPUDescriptorAllocator shaderResourceDescriptorAllocator_;
	};

}