#include "render/command_queue.h"

#include "render/command_context.h"

#include "core/debug.h"

#include "render/device.h"

namespace render {

	CommandQueue::CommandQueue(Device* device, CommandQueue::Type type)
		: DeviceObject(device), type_(type), fence_(device, "CommandQueue Fence")
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		switch (type)
		{
		case CommandQueue::GRAPHICS:
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			break;
		case CommandQueue::COPY:
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
			break;
		default:
			ASSERT(false)
			break;
		}
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		ThrowIfFailed(device->device()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_)));
	}

	SyncPoint CommandQueue::execute(CommandContext* context)
	{
		// TODO : multithreading, add resolve resource transition  
		// Commandlists can be recorded in parallel.
		context->flushResourceBarriers();
		context->close();
		ID3D12CommandList* cmdsLists[] = { context->list() };
		commandQueue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		size_t fenceValue = fence_.signal(this);
		syncPoint_ = SyncPoint(&fence_, fenceValue);

		context->free(syncPoint_);

		return syncPoint_;
	}
}