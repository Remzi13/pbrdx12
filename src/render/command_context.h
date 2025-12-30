#pragma once

#include "core/std_types.h"

#include "render/command_queue.h"
#include "render/sync_point.h"
#include "render/descriptor_heap.h"
#include "render/page_allocator.h"

namespace render {

	class Device;
	class Buffer;
	class PageAllocator;
	class RootSignature;
	class PipelineState;

	class CommandContext : public DeviceObject
	{
	public:

		//struct RenderPassInfo
		//{
		//	struct RenderTarget
		//	{
		//		Texture* texture{ nullptr };
		//		uint8 mipLevel = 0;
		//		RenderPassColorFlags Flags = RenderPassColorFlags::None;
		//	} renderTarget;
		//
		//	struct DepthTarget
		//	{
		//		Texture* texture{ nullptr };
		//		uint8 mipLevel = 0;
		//		RenderPassDepthFlags Flags = RenderPassDepthFlags::None;
		//		struct Data
		//		{
		//			Data(float depth = 0.0f, uint8 stencil = 1)
		//				: Depth(depth), Stencil(stencil)
		//			{
		//			}
		//			float Depth;
		//			uint8 Stencil;
		//		} data;
		//
		//	} depthTarget;
		//
		//
		//	RenderPassInfo() = default;
		//	RenderPassInfo(Texture* target, Texture* depthStencil, RenderPassColorFlags colorFlag = RenderPassColorFlags::Clear)
		//		: renderTarget({ target, 0, colorFlag })
		//	{
		//		if (depthStencil)
		//		{
		//			depthTarget.texture = depthStencil;
		//			depthTarget.Flags = RenderPassDepthFlags::Clear;
		//			depthTarget.data.Depth = depthStencil->clearColor().r;
		//			depthTarget.data.Stencil = static_cast<uint8>(depthStencil->clearColor().g);
		//		}
		//	}
		//};
		CommandContext(Device* device, CommandQueue::Type type, GPUDescriptorHeap* descriptorHeap, PageAllocator* pageAllocator);
		~CommandContext() {}

		//virtual void begin(RenderPassInfo info) = 0;
		//virtual void end() = 0;
		void reset();
		void flushResourceBarriers();
		//virtual void insertResourceBarrier(Texture* pResource, ResourceState beforeState, ResourceState afterState, uint32 subResource) = 0;
		void copyBuffer(const Buffer* pSource, const Buffer* pTarget, uint64 size, uint64 sourceOffset, uint64 destinationOffset);
		//virtual void addResourceBarrier(Texture* pResource, ResourceState beforeState, ResourceState afterState, uint32 subResource) = 0;
		//virtual void setRootSignature(const RootSignature* pRootSignature) = 0;
		//virtual void setPipelineState(const PipelineState* pipelineState) = 0;
		//virtual void setPrimitiveTopology(const PrimitiveTopology topology) = 0;
		//virtual void setViewport(const math::RectF& rect, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
		//virtual device::Allocation allocate(uint64 size, uint32 alignment = 16u) = 0;
		//virtual void setVertexBuffer(Buffer::VertexView view) = 0;
		//virtual void setIndexBuffer(Buffer::IndexView view) = 0;
		//virtual void setScissorRect(const math::RectF& rect) = 0;
		//virtual void bindRootCBV(uint32 rootIndex, const void* data, uint32 size) = 0;
		//virtual void bindResources(uint32 rootIndex, const Buffer* pViews, uint32 offset = 0) = 0;
		SyncPoint execute();
		//
		//virtual void drawIndexedInstanced(uint32 indexCount, uint32 indexStart, uint32 instanceCount, uint32 minVertex = 0, uint32 instanceStart = 0) = 0;
		//
		void free(SyncPoint syncPoint);
		//
		//CommandQueue::Type type() const { return type_; }
		bool isComplete() const { return syncPoint_.isComplete(); }

		void close();
		ID3D12GraphicsCommandList* list() const { return commandList_.Get(); }
	private:
		void clearState();

	private:
		CommandQueue::Type type_;
		SyncPoint syncPoint_;

		uint32 numBatchedBarriers_{ 0 };

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
		unsigned int rtvDescriptorSize_;

		const RootSignature* currentRootSignature_{ nullptr };
		const PipelineState* currentPipelineState_{ nullptr };


		Allocator allocator_;
		GPUDescriptorAllocator shaderResourceDescriptorAllocator_;
	};
}