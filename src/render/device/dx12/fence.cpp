#include "render/device/dx12/fence.h"

#include "render/device/dx12/device.h"
#include "render/device/dx12/command_queue.h"

namespace elm::render::device {

	Fence::Fence(GraphicsDevice* device, const char* name, uint64 value)
		: DeviceObject(device), currentValue_(value +1), lastSignaled_(0), lastCompleted_(value)
	{
		ThrowIfFailed(static_cast<DeviceDx12*>(parent())->device()->CreateFence(lastCompleted_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.GetAddressOf())));
		SetObjectName(fence_.Get(), name);
		completeEvent_ = CreateEventExA(nullptr, "Fence Event", 0, EVENT_ALL_ACCESS);
	}

	Fence::~Fence()
	{}	

	uint64 Fence::signal(CommandQueue* pQueue)
	{
		static_cast<CommandQueueDx12*>(pQueue)->commandQueue()->Signal(fence_.Get(), currentValue_);
		lastSignaled_ = currentValue_;
		currentValue_++;
		return lastSignaled_;
	}

	void Fence::cpuWait(uint64 fenceValue)
	{
		if (isComplete(fenceValue))
		{
			return;
		}

		//std::lock_guard<std::mutex> lockGuard(m_FenceWaitCS);

		fence_->SetEventOnCompletion(fenceValue, completeEvent_);
		DWORD result = WaitForSingleObject(completeEvent_, INFINITE);

#ifdef USE_PIX
		// The event was successfully signaled, so notify PIX
		if (result == WAIT_OBJECT_0)
		{
			PIXNotifyWakeFromFenceSignal(m_CompleteEvent);
		}
#else
		UNREFERENCED_PARAMETER(result);
#endif

		lastCompleted_ = fence_->GetCompletedValue();
	}

	void Fence::cpuWait()
	{
		cpuWait(lastSignaled_);
	}
}

namespace elm::render {
	void SyncPoint::wait() const
	{
		fence_->cpuWait(value_);
	}
	bool SyncPoint::isComplete() const
	{
		return fence_ && fence_->isComplete(value_);
	}
}