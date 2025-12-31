#pragma once

#include "core/std_types.h"

#include "render/device/device_interface.h"

namespace render::device {
	
	class Fence : public DeviceObject
	{
	public:
		Fence(GraphicsDevice* device, const char* pName, uint64 fenceValue = 0);
		~Fence();
				
		uint64 signal(CommandQueue* pQueue);
		uint64 signal(uint64 fenceValue);
		// Stall CPU until fence value is signaled on the GPU
		void cpuWait(uint64 fenceValue);
		void cpuWait();
		
		bool isComplete(uint64 fenceValue)
		{
			if (fenceValue <= lastCompleted_)
			{
				return true;
			}
			lastCompleted_ = core::Max(lastCompleted_, fence_->GetCompletedValue());
			return fenceValue <= lastCompleted_;
		}


		uint64 currentValue() const { return currentValue_; }
		uint64 lastCompleted() const { return lastCompleted_; }
		//uint64 GetLastSignaledValue() const { return m_LastSignaled; }

		inline ID3D12Fence* fence() const { return fence_.Get(); }

	private:
		Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
		//std::mutex m_FenceWaitCS;
		HANDLE completeEvent_;
		uint64 currentValue_;
		uint64 lastSignaled_;
		uint64 lastCompleted_;
	};

}