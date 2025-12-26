#include "render/command_context.h"

#include "render/common.h"

#include "core/std_types.h"

namespace render {

	
	void CommandContext::flushResourceBarriers()
	{
		//if (numBatchedBarriers_ > 0)
		//{
		//	commandList_->ResourceBarrier(numBatchedBarriers_, batchedBarriers_.data());
		//	numBatchedBarriers_ = 0;
		//}
	}

	void CommandContext::free(SyncPoint syncPoint)
	{
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

}