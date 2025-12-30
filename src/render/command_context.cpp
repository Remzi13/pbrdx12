#include "render/command_context.h"

#include "core/std_types.h"
#include "core/debug.h"


#include "render/common.h"
#include "render/resources.h"
#include "render/device.h"

namespace render {

	namespace {
		constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_UNKNOWN = (D3D12_RESOURCE_STATES)-1;

		D3D12_COMMAND_LIST_TYPE convertType(CommandQueue::Type type)
		{
			switch (type)
			{
			case CommandQueue::GRAPHICS: return D3D12_COMMAND_LIST_TYPE_DIRECT;
			case CommandQueue::COPY: return D3D12_COMMAND_LIST_TYPE_COPY;
			default:
				ASSERT(false);
				break;
			}
			return D3D12_COMMAND_LIST_TYPE_NONE;
		}
	}

	CommandContext::CommandContext(Device* device, CommandQueue::Type type, GPUDescriptorHeap* descriptorHeap, PageAllocator* pageAllocator)
		: DeviceObject(device), type_(type), shaderResourceDescriptorAllocator_(descriptorHeap)
	{
		
		ThrowIfFailed(device->device()->CreateCommandAllocator(
			convertType(type),
			IID_PPV_ARGS(commandAllocator_.GetAddressOf())));

		ThrowIfFailed(device->device()->CreateCommandList(
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
		ThrowIfFailed(device->device()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(rtvHeap_.GetAddressOf())));
		rtvDescriptorSize_ = device->device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		ThrowIfFailed(device->device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(dsvHeap_.GetAddressOf())));
		// Start off in a closed state.  This is because the first time we refer 
		// to the command list we will Reset it, and it needs to be closed before
		// calling Reset.
		commandList_->Close();

		allocator_.init(pageAllocator);
	}

	void CommandContext::copyBuffer(const Buffer* source, const Buffer* target, uint64 size, uint64 sourceOffset, uint64 destinationOffset)
	{			
		ASSERT(source && source->resource(), "Source is invalid");
		ASSERT(target && target->resource(), "Target is invalid");

		flushResourceBarriers();

		commandList_->CopyBufferRegion(target->resource(), destinationOffset, source->resource(), sourceOffset, size);
	}
	
	void CommandContext::flushResourceBarriers()
	{
		ASSERT(false);
		//if (numBatchedBarriers_ > 0)
		//{
		//	commandList_->ResourceBarrier(numBatchedBarriers_, batchedBarriers_.data());
		//	numBatchedBarriers_ = 0;
		//}
	}

	void CommandContext::free(SyncPoint syncPoint)
	{
		ASSERT(false);
		//allocator_.free(syncPoint);
		//
		//if (type_ != CommandQueue::COPY)
		//{
		//	shaderResourceDescriptorAllocator_.releaseUsedHeaps(syncPoint);
		//}
	}

	void CommandContext::close()
	{
		commandList_->Close();
	}

	void CommandContext::reset()
	{
		ThrowIfFailed(commandAllocator_->Reset());
		ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), nullptr));

		currentPipelineState_ = nullptr;
		currentRootSignature_ = nullptr;
		clearState();
	}

	void CommandContext::clearState()
	{
		if (type_ != CommandQueue::COPY)
		{
			flushResourceBarriers();

			commandList_->ClearState(nullptr);

			ID3D12DescriptorHeap* pHeaps[] =
			{
				parent()->globalViewHeap()->heap(),
				parent()->globalSamplerHeap()->heap(),
			};
			commandList_->SetDescriptorHeaps(ARRAYSIZE(pHeaps), pHeaps);
		}
	}

	SyncPoint CommandContext::execute()
	{
		CommandQueue* queue = parent()->commandQueue(type_);
		syncPoint_ = queue->execute(this);
		return syncPoint_;
	}

}