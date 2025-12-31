#pragma once

#include "core/std_types.h"
#include "core/memory.h"

#include "render/device/texture.h"
#include "render/device/device_interface.h"

namespace elm::render {

	class RGPass
	{
	public:
		RGPass(const char* name);

		bool execute(CommandContext* context);

		void renderTarget(Texture* resource, RenderPassColorFlags colorFlag);
		void depthStencil(Texture* resource);
		void bind(std::function<bool(CommandContext*)> callback);

	private:
		string name_;
		std::pair<Texture*, RenderPassColorFlags> renderTarget_;
		Texture* depthStencil_;
		std::function<bool(CommandContext*)> callback_;
	};

	class RGraph
	{
	public:
		RGraph();

		RGPass* addPass(const char* name);
		bool execute(GraphicsDevice* device);

	private:
		vector<std::unique_ptr<RGPass>> passes_;
	};
}
