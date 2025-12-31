#include "render/rgraph.h"

namespace elm::render {

	RGPass::RGPass(const char* name) : name_(name)
	{}

	bool RGPass::execute(CommandContext* context)
	{		
		context->insertResourceBarrier(renderTarget_.first, ResourceState::Common, ResourceState::RenderTarget, 0xffffffff);
		context->insertResourceBarrier(depthStencil_, ResourceState::Common, ResourceState::Depth, 0xffffffff);

		context->begin(CommandContext::RenderPassInfo(renderTarget_.first, depthStencil_, renderTarget_.second));
		context->setScissorRect(math::RectF(0, 0, (float)renderTarget_.first->width(), (float)renderTarget_.first->height()));

		auto result = callback_(context);

		context->insertResourceBarrier(renderTarget_.first, ResourceState::RenderTarget, ResourceState::Common, 0xffffffff);
		context->insertResourceBarrier(depthStencil_, ResourceState::Depth, ResourceState::Common, 0xffffffff);
		context->end();
		return result;
	}

	void RGPass::renderTarget(Texture* resource, RenderPassColorFlags colorFlag)
	{
		renderTarget_ = std::make_pair(resource, colorFlag);
	}

	void RGPass::depthStencil(Texture* resource)
	{
		depthStencil_ = resource;
	}

	void RGPass::bind(std::function<bool(CommandContext*)> callback)
	{
		callback_ = callback;
	}

	RGraph::RGraph()
	{
	}

	RGPass* RGraph::addPass(const char* name)
	{		
		passes_.push_back(memory::makeUnique<RGPass>(name));
		return passes_.back().get();
	}

	bool RGraph::execute(GraphicsDevice* device)
	{
		bool result = false;
		auto context = device->getCommandContext(CommandQueue::GRAPHICS);

		context->setRootSignature(getRootSignature(RootSignatureType::COMMON, nullptr).get());
		context->setPrimitiveTopology(PrimitiveTopology::TriangleList);

		for (const auto& pass : passes_)
		{
			result &= pass->execute(context);
		}
		context->execute();

		device->backCommandContext(context);
		passes_.clear();

		return result;
	}
}