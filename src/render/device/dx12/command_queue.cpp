#include "render/device/dx12/command_queue.h"

#include "render/utils.h"

#include "render/device/dx12/device.h"
#include "render/device/dx12/command_context.h"

namespace elm::render {

	CommandQueueDx12::CommandQueueDx12(DeviceDx12* device, CommandQueue::Type type) 
		: CommandQueue(device, type), fence_(device, "CommandQueue Fence")
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		switch (type)
		{
		case elm::render::CommandQueue::GRAPHICS:
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			break;
		case elm::render::CommandQueue::COPY:
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
			break;
		default:
			ELM_ASSERT(false)
			break;
		}		
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
				
		ThrowIfFailed(device->device()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_)));
	}

	SyncPoint CommandQueueDx12::execute(CommandContext* context)
	{
		// TODO : multithreading, add resolve resource transition  
		// Commandlists can be recorded in parallel.
		context->flushResourceBarriers();
		static_cast<CommandContextDx12*>(context)->list()->Close();
		ID3D12CommandList* cmdsLists[] = { static_cast<CommandContextDx12*>(context)->list()};
		commandQueue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		uint64 fenceValue = fence_.signal(this);
		syncPoint_ = SyncPoint(&fence_, fenceValue);

		context->free(syncPoint_);

		return syncPoint_;
	}
}