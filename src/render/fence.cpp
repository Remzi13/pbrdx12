#include "render/fence.h"

#include "core/std_types.h"

#include "render/device.h"


namespace render {

	Fence::Fence(Device* device, const char* name, size_t value)
		: DeviceObject(device), currentValue_(value + 1), lastSignaled_(0), lastCompleted_(value)
	{		
		//parent()->device()->CreateFence(lastCompleted_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.GetAddressOf()));
		ThrowIfFailed(parent()->device()->CreateFence(lastCompleted_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.GetAddressOf())));
		SetObjectName(fence_.Get(), name);
		completeEvent_ = CreateEventExA(nullptr, "Fence Event", 0, EVENT_ALL_ACCESS);
	}

	Fence::~Fence()
	{
	}

	size_t Fence::signal(CommandQueue* pQueue)
	{
		//static_cast<CommandQueueDx12*>(pQueue)->commandQueue()->Signal(fence_.Get(), currentValue_);
		lastSignaled_ = currentValue_;
		currentValue_++;
		return lastSignaled_;
	}

	void Fence::cpuWait(size_t fenceValue)
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

	bool Fence::isComplete(size_t fenceValue)  
	{
		if (fenceValue <= lastCompleted_)
		{
			return true;
		}
		lastCompleted_ = max(lastCompleted_, (uint64)fence_->GetCompletedValue());
		return fenceValue <= lastCompleted_;
	}


	void Fence::cpuWait()
	{
		cpuWait(lastSignaled_);
	}
}