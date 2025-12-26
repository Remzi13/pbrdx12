#pragma once

#include "render/common.h"

namespace render {

	class CommandQueue;

	class Fence : public DeviceObject
	{
	public:
		Fence(Device* device, const char* pName, size_t fenceValue = 0);
		~Fence();

		size_t signal(CommandQueue* pQueue);
		size_t signal(size_t fenceValue);
		// Stall CPU until fence value is signaled on the GPU
		void cpuWait(size_t fenceValue);
		void cpuWait();

		bool isComplete(size_t fenceValue) ;

		size_t currentValue() const { return currentValue_; }
		size_t lastCompleted() const { return lastCompleted_; }
		//uint64 GetLastSignaledValue() const { return m_LastSignaled; }

		inline ID3D12Fence* fence() const { return fence_.Get(); }

	private:
		Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
		//std::mutex m_FenceWaitCS;
		HANDLE completeEvent_;
		size_t currentValue_;
		size_t lastSignaled_;
		uint64 lastCompleted_;
	};

}