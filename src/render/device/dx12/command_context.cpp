#include "render/device/dx12/command_context.h"

#include "render/device/dx12/device.h"
#include "render/device/dx12/root_signature.h"
#include "render/device/dx12/pipeline_state.h"
#include "render/device/dx12/resource.h"
#include "render/utils.h"

#include <DirectXColors.h>

namespace render {

	namespace {

		constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_UNKNOWN = (D3D12_RESOURCE_STATES)-1;

		D3D12_COMMAND_LIST_TYPE convertType(CommandQueue::Type type)
		{
			switch (type)
			{
			case render::CommandQueue::GRAPHICS: return D3D12_COMMAND_LIST_TYPE_DIRECT;
			case render::CommandQueue::COPY: return D3D12_COMMAND_LIST_TYPE_COPY;
			default:
				ASSERT(false);
				break;
			}
			return D3D12_COMMAND_LIST_TYPE_NONE;
		}

		bool HasWriteResourceState(D3D12_RESOURCE_STATES state)
		{
			return EnumHasAnyFlags(state,
				D3D12_RESOURCE_STATE_STREAM_OUT |
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
				D3D12_RESOURCE_STATE_RENDER_TARGET |
				D3D12_RESOURCE_STATE_DEPTH_WRITE |
				D3D12_RESOURCE_STATE_COPY_DEST |
				D3D12_RESOURCE_STATE_RESOLVE_DEST |
				D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE |
				D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE |
				D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE
			);
		};

		bool CanCombineResourceState(D3D12_RESOURCE_STATES stateA, D3D12_RESOURCE_STATES stateB)
		{
			return !HasWriteResourceState(stateA) && !HasWriteResourceState(stateB);
		}
		bool NeedsTransition(D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES& after, bool allowCombine)
		{
			if (before == after)
				return false;

			// When resolving pending resource barriers, combining resource states is not working
			// This is because the last known resource state of the resource is used to update the resource
			// And so combining after_state during the result will result in the last known resource state not matching up.
			if (!allowCombine)
				return true;

			//Can read from 'write' DSV
			if (before == D3D12_RESOURCE_STATE_DEPTH_WRITE && after == D3D12_RESOURCE_STATE_DEPTH_READ)
				return false;

			if (after == D3D12_RESOURCE_STATE_COMMON)
				return before != D3D12_RESOURCE_STATE_COMMON;

			//Combine already transitioned bits
			if (CanCombineResourceState(before, after) && !EnumHasAllFlags(before, after))
				after |= before;

			return true;
		}
	}

	CommandContextDx12::CommandContextDx12(DeviceDx12* device, CommandQueue::Type type, GPUDescriptorHeap* descriptorHeap, device::PageAllocator* pageAllocator)
		: CommandContext(device, type), shaderResourceDescriptorAllocator_(descriptorHeap)
	{
		deviceX_ = device->device();

		ThrowIfFailed(deviceX_->CreateCommandAllocator(
			convertType(type),
			IID_PPV_ARGS(commandAllocator_.GetAddressOf())));
		
		ThrowIfFailed(deviceX_->CreateCommandList(
			0,
			convertType(type),
			commandAllocator_.Get(), // Associated command allocator
			nullptr,                   // Initial PipelineStateObject
			IID_PPV_ARGS(commandList_.GetAddressOf())));
		//ThrowIfFailed(deviceX_->CreateCommandList1(0, convertType(type), D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(commandList_.GetAddressOf())));

		// TODO: move to CommandContext		
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
		rtvHeapDesc.NumDescriptors = 3;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		ThrowIfFailed(deviceX_->CreateDescriptorHeap( &rtvHeapDesc, IID_PPV_ARGS(rtvHeap_.GetAddressOf())));
		rtvDescriptorSize_ = deviceX_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		ThrowIfFailed(deviceX_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(dsvHeap_.GetAddressOf())));
		// Start off in a closed state.  This is because the first time we refer 
		// to the command list we will Reset it, and it needs to be closed before
		// calling Reset.
		commandList_->Close();

		allocator_.init(pageAllocator);
	}
	
	void CommandContextDx12::begin(RenderPassInfo info)
	{
		ASSERT(!inRenderPass_, "Already in RenderPass");

		flushResourceBarriers();

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};

		D3D12_CLEAR_FLAGS clearFlags = (D3D12_CLEAR_FLAGS)0;
		if (EnumHasAllFlags(info.depthTarget.Flags, RenderPassDepthFlags::ClearDepth))
			clearFlags |= D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH;

		if (EnumHasAllFlags(info.depthTarget.Flags, RenderPassDepthFlags::ClearStencil))
			clearFlags |= D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_STENCIL;

		if (info.depthTarget.texture)
		{
			dsvHandle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			const TextureDesc& desc = info.depthTarget.texture->desc();
			dsvDesc.Format = convertFormat(desc.Format);
			switch (desc.Type)
			{			
			case TextureType::Texture2D:
				dsvDesc.Texture2D.MipSlice = info.depthTarget.mipLevel;
				dsvDesc.ViewDimension = desc.SampleCount > 1 ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
				break;			
			default:
				ASSERT(false, "Unsupported RenderTarget ")
				break;
			}
			if (EnumHasAllFlags(info.depthTarget.Flags, RenderPassDepthFlags::ReadOnlyDepth))
				dsvDesc.Flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
			if (EnumHasAllFlags(info.depthTarget.Flags, RenderPassDepthFlags::ReadOnlyStencil))
				dsvDesc.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;
			TextureDx12* depthTarget= static_cast<TextureDx12*>(info.depthTarget.texture);
			deviceX_->CreateDepthStencilView(depthTarget->resource(), &dsvDesc, dsvHandle);
		}

		if (clearFlags != (D3D12_CLEAR_FLAGS)0)
		{
			const auto& clearBinding = info.depthTarget.texture->clearColor();
			//gAssert(clearBinding.BindingValue == ClearBinding::ClearBindingValue::DepthStencil);
			//commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
			commandList_->ClearDepthStencilView(dsvHandle, clearFlags, info.depthTarget.data.Depth, info.depthTarget.data.Stencil, 0, nullptr);
		}

		array<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT> rtvs;
		const int renderTargetId = 0;
		
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		const TextureDesc& desc = info.renderTarget.texture->desc();
		rtvDesc.Format = convertFormat(desc.Format);
		switch (desc.Type)
		{			
			case TextureType::Texture2D:
				rtvDesc.Texture2D.MipSlice = info.renderTarget.mipLevel;
				rtvDesc.Texture2D.PlaneSlice = 0;
				rtvDesc.ViewDimension = desc.SampleCount > 1 ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;
				break;			
			default:
				ASSERT(false, "Unsupported RenderTarget ")
				break;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap_->GetCPUDescriptorHandleForHeapStart(), renderTargetId, rtvDescriptorSize_);
		TextureDx12* renderTarget = static_cast<TextureDx12*>(info.renderTarget.texture);
		deviceX_->CreateRenderTargetView(renderTarget->resource(), &rtvDesc, rtv);

		if (EnumHasAnyFlags(info.renderTarget.Flags, RenderPassColorFlags::Clear))
		{
			//ASSERT(data.pTarget->GetClearBinding().BindingValue == ClearBinding::ClearBindingValue::Color);
			FLOAT colorRGBA[4];
			convertFormat(info.renderTarget.texture->clearColor(), colorRGBA);
			commandList_->ClearRenderTargetView(rtv, colorRGBA, 0, nullptr);
		}
		rtvs[renderTargetId] = rtv;

		commandList_->OMSetRenderTargets(1, rtvs.data(), false, dsvHandle.ptr != 0 ? &dsvHandle : nullptr);
		
		setViewport(core::RectF(0, 0, (float)renderTarget->width(), (float)renderTarget->height()), 0, 1);
		inRenderPass_ = true;
	}

	void CommandContextDx12::end()
	{		
		if (EnumHasAllFlags(currentPass_.renderTarget.Flags, RenderPassColorFlags::Resolve))
		{
			ASSERT(false);
			if (currentPass_.renderTarget.texture->desc().SampleCount > 1)
			{				
				//TextureDx12* renderTarget = static_cast<TextureDx12*>(currentPass_.renderTarget.texture);
				//insertResourceBarrier(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
				//insertResourceBarrier(data.pResolveTarget, D3D12_RESOURCE_STATE_UNKNOWN, D3D12_RESOURCE_STATE_RESOLVE_DEST);
				//uint32 subResource = D3D12CalcSubresource(data.MipLevel, data.ArrayIndex, 0, data.pTarget->GetMipLevels(), data.pTarget->GetArraySize());
				//ResolveResource(data.pTarget, subResource, data.pResolveTarget, 0, data.pTarget->GetFormat());
			}
			//else if (data.pTarget != data.pResolveTarget)
			//{
			//	//E_LOG(Warning, "RenderTarget %u is set to resolve but has a sample count of 1. This will just do a CopyTexture instead which is wasteful.", i);
			//	CopyResource(data.pTarget, data.pResolveTarget);
			//}
		}		

		inRenderPass_ = false;
	}

	void CommandContextDx12::addBarrier(const D3D12_RESOURCE_BARRIER& barrier)
	{
		batchedBarriers_[numBatchedBarriers_++] = barrier;
		if (numBatchedBarriers_ >= MAX_BATCHED_BARRIERS)
			flushResourceBarriers();
	}

	void CommandContextDx12::flushResourceBarriers()
	{
		if (numBatchedBarriers_ > 0)
		{
			commandList_->ResourceBarrier(numBatchedBarriers_, batchedBarriers_.data());
			numBatchedBarriers_ = 0;
		}
	}

	void CommandContextDx12::copyBuffer(const Buffer* source, const Buffer* traget, uint64 size, uint64 sourceOffset, uint64 destinationOffset)
	{
		const auto t = static_cast<const BufferDx12*>(traget);
		const auto s = static_cast<const BufferDx12*>(source);
		ASSERT(s && s->resource(), "Source is invalid");
		ASSERT(t && t->resource(), "Target is invalid");
		
		flushResourceBarriers();
		
		commandList_->CopyBufferRegion(t->resource(), destinationOffset, s->resource(), sourceOffset, size);
	}

	void CommandContextDx12::reset()
	{
		ThrowIfFailed(commandAllocator_->Reset());
		ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), nullptr));

		currentPipelineState_ = nullptr;
		currentRootSignature_ = nullptr;
		clearState();
	}

	void CommandContextDx12::clearState()
	{
		if (type_ != CommandQueue::COPY)
		{
			flushResourceBarriers();

			commandList_->ClearState(nullptr);

			ID3D12DescriptorHeap* pHeaps[] =
			{
				static_cast<DeviceDx12*>(parent())->globalViewHeap()->heap(),
				static_cast<DeviceDx12*>(parent())->globalSamplerHeap()->heap(),
			};
			commandList_->SetDescriptorHeaps(ARRAYSIZE(pHeaps), pHeaps);
		}
	}

	void CommandContextDx12::insertResourceBarrier(Texture* resource, ResourceState beforeState, ResourceState afterState, uint32 subResource)
	{
		auto raw = static_cast<TextureDx12*>(resource);
		if (raw->resourceState() == afterState)
		{
			return;
		}

		addBarrier(CD3DX12_RESOURCE_BARRIER::Transition(raw->resource(),
			convertFormat(beforeState),
			convertFormat(afterState),
			subResource,
			D3D12_RESOURCE_BARRIER_FLAG_NONE)
		);
		raw->setResourceState(afterState);
		flushResourceBarriers();
	}

	void CommandContextDx12::addResourceBarrier(Texture* texture, ResourceState beforeState, ResourceState afterState, uint32 subResource)
	{
		CD3DX12_RESOURCE_BARRIER transition;
		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			dynamic_cast<TextureDx12 *>(texture)->resource(), convertFormat(beforeState), convertFormat(afterState));
		commandList_->ResourceBarrier(1, &transition);
	}

	ID3D12GraphicsCommandList* CommandContextDx12::list() const
	{
		return commandList_.Get();
	}

	void CommandContextDx12::setViewport(const core::RectF& rect, float minDepth, float maxDepth )
	{
		D3D12_VIEWPORT viewport = {
			.TopLeftX = rect.Left,
			.TopLeftY = rect.Top,
			.Width = rect.width(),
			.Height = rect.height(),
			.MinDepth = minDepth,
			.MaxDepth = maxDepth,
		};

		commandList_->RSSetViewports(1, &viewport);
		setScissorRect(rect);
	}

	void CommandContextDx12::setScissorRect(const core::RectF& rect)
	{
		D3D12_RECT r = {
			.left = (LONG)rect.Left,
			.top = (LONG)rect.Top,
			.right = (LONG)rect.Right,
			.bottom = (LONG)rect.Bottom,
		};

		commandList_->RSSetScissorRects(1, &r);
	}

	void CommandContextDx12::setRootSignature(const RootSignature* rootSignature)
	{
		if (currentRootSignature_ != rootSignature)
		{
			commandList_->SetGraphicsRootSignature(static_cast<const RootSignatureDx12*>(rootSignature)->raw());
			// TODO 
			//m_ShaderResourceDescriptorAllocator.ParseRootSignature(pRootSignature);
			currentRootSignature_ = rootSignature;
		}
	}

	void CommandContextDx12::setPipelineState(const PipelineState* pipelineState) 
	{
		if (currentPipelineState_ != pipelineState)
		{
			commandList_->SetPipelineState(static_cast<const PipelineStateDx12*>(pipelineState)->raw());
			currentPipelineState_ = pipelineState;
		}
	}

	void CommandContextDx12::setPrimitiveTopology(const PrimitiveTopology topology)
	{
		commandList_->IASetPrimitiveTopology(convertFormat(topology));
	}

	device::Allocation CommandContextDx12::allocate(uint64 size, uint32 alignment )
	{
		return allocator_.allocate(size, alignment);
	}

	void CommandContextDx12::free(SyncPoint syncPoint)
	{
		allocator_.free(syncPoint);

		if (type_ != CommandQueue::COPY)
		{
			shaderResourceDescriptorAllocator_.releaseUsedHeaps(syncPoint);
		}
	}

	void CommandContextDx12::setVertexBuffer(Buffer::VertexView view)
	{		
		constexpr uint32 numViews = 1;
		D3D12_VERTEX_BUFFER_VIEW views[] = { 
			{
				.BufferLocation = view.Location,
				.SizeInBytes = view.Elements * view.Stride,
				.StrideInBytes = view.Stride,
			} 
		};
		
		commandList_->IASetVertexBuffers(0, numViews, views);
	}

	void CommandContextDx12::setIndexBuffer(Buffer::IndexView indexView)
	{
		D3D12_INDEX_BUFFER_VIEW view = {
			.BufferLocation = indexView.Location,
			.SizeInBytes = formatInfo(indexView.Format).BytesPerBlock * indexView.Elements,
			.Format = convertFormat(indexView.Format),
		};

		commandList_->IASetIndexBuffer(&view);
	}

	void CommandContextDx12::bindRootCBV(uint32 rootIndex, const void* data, uint32 dataSize)
	{
		// todo: add compute command list

		const RootSignatureDx12* pRootSignature = static_cast<const RootSignatureDx12*>(currentRootSignature_);
		bool isRootConstants = pRootSignature->isRootConstant(rootIndex);
		if (isRootConstants)
		{
			ASSERT(dataSize % sizeof(uint32) == 0);
			uint32 rootConstantsSize = pRootSignature->numRootConstants(rootIndex) * sizeof(uint32);
			ASSERT(dataSize <= rootConstantsSize);

#ifdef _DEBUG
			// In debug, write 0xCDCDCDCD to unwritten root constants
			if (rootConstantsSize != dataSize)
			{
				void* pLocalData = _alloca(rootConstantsSize);
				memset(pLocalData, (int)0xCDCDCDCD, rootConstantsSize);
				memcpy(pLocalData, data, dataSize);
				dataSize = rootConstantsSize;
				data = pLocalData;
			}
#endif

			if (type_ == CommandQueue::Type::GRAPHICS)
			{
				commandList_->SetGraphicsRoot32BitConstants(rootIndex, dataSize / sizeof(uint32), data, 0);
			}
			else
			{
				commandList_->SetComputeRoot32BitConstants(rootIndex, dataSize / sizeof(uint32), data, 0);
			}
		}
		else
		{
			device::Allocation allocation = allocate(dataSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
			memcpy(allocation.mappedMemory, data, dataSize);

			ASSERT(!pRootSignature->isRootConstant(rootIndex));
			//if (m_CurrentCommandContext == CommandListContext::Graphics)
			//	m_pCommandList->SetGraphicsRootConstantBufferView(rootIndex, allocation.GpuHandle);
			//else
			//	m_pCommandList->SetComputeRootConstantBufferView(rootIndex, allocation.GpuHandle);
			commandList_->SetGraphicsRootConstantBufferView(rootIndex, allocation.Location);
		}
	}

	void CommandContextDx12::bindResources(uint32 rootIndex, const Buffer* pViews, uint32 offset )
	{
		shaderResourceDescriptorAllocator_.setDescriptors(rootIndex, offset, static_cast<const BufferDx12*>(pViews)->srv());
	}

	void CommandContextDx12::drawIndexedInstanced(uint32 indexCount, uint32 indexStart, uint32 instanceCount, uint32 minVertex, uint32 instanceStart)
	{
		ASSERT(currentPipelineState_);
		//gAssert(m_CurrentCommandContext == CommandListContext::Graphics);
		prepareDraw();
		commandList_->DrawIndexedInstanced(indexCount, instanceCount, indexStart, minVertex, instanceStart);
	}

	void CommandContextDx12::prepareDraw()
	{
		flushResourceBarriers();
		shaderResourceDescriptorAllocator_.bindStagedDescriptors(*this);
	}

	SyncPoint CommandContextDx12::execute() 
	{
		CommandQueue* queue = parent()->commandQueue(type_);
		syncPoint_ = queue->execute(this);
		return syncPoint_;
	}
}