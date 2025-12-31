#pragma once

#include "render/device/device_interface.h"

#include "render/device/dx12/fence.h"

struct ID3D12CommandQueue;

namespace render {

	class DeviceDx12;

	class CommandQueueDx12 : public CommandQueue
	{
	public:
		CommandQueueDx12(DeviceDx12* device, CommandQueue::Type type);

		SyncPoint execute(CommandContext* context) override;

		[[nodiscard]] ID3D12CommandQueue* commandQueue() const { return commandQueue_; };

	private:
		ID3D12CommandQueue* commandQueue_;
		SyncPoint syncPoint_;
		device::Fence fence_;
	};


}