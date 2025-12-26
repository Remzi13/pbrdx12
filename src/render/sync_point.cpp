#include "render/sync_point.h"

#include "render/fence.h"

namespace render {

	void SyncPoint::wait() const
	{
		fence_->cpuWait(value_);
	}
	bool SyncPoint::isComplete() const
	{
		return fence_ && fence_->isComplete(value_);
	}
}