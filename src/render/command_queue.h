#pragma once

#include "render/common.h"

#include "render/fence.h"
#include "render/sync_point.h"

namespace render {

	class Device;
	class CommandContext;

	class CommandQueue : public DeviceObject
	{
	public:
		enum Type
		{
			GRAPHICS, //D3D12_COMMAND_LIST_TYPE_DIRECT
			COPY,
			Count
		};

		CommandQueue(Device* device, Type type);

		~CommandQueue() override = default;

		SyncPoint execute(CommandContext* context);

	private:
		Type type_;

		ID3D12CommandQueue* commandQueue_;
		SyncPoint syncPoint_;
		Fence fence_;
	};
}