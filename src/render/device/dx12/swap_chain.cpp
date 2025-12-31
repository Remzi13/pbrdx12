#include "render/device/dx12/swap_chain.h"

#include "render/utils.h"

#include "render/device/dx12/device.h"
#include "render/device/dx12/command_queue.h"


namespace elm::render {

	SwapChainDx12::SwapChainDx12(DeviceDx12* device, HWND hwnd, int width, int height, int numFrames, ResourceFormat format)
		: SwapChain(device, numFrames), hwnd_(hwnd), format_(format)
	{
		nFence_ = makeUnique<device::Fence>(device, "SwapChain Fence");

		DXGI_SWAP_CHAIN_DESC sd;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = convertFormat(format_);
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = numFrames_;
		sd.OutputWindow = hwnd_;
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		CommandQueueDx12* rawCommandQueue = static_cast<CommandQueueDx12*>(parent()->commandQueue(CommandQueue::GRAPHICS));
		auto t = rawCommandQueue->commandQueue();
		// Note: Swap chain uses queue to perform flush.
		ThrowIfFailed(static_cast<DeviceDx12*>(parent())->factory()->CreateSwapChain(t, &sd, &swapChain_));

		//Recreate the render target views
		for (uint32 i = 0; i < numFrames_; ++i)
		{
			ID3D12ResourceX* pResource = nullptr;
			ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&pResource)));
			D3D12_RESOURCE_DESC resourceDesc = pResource->GetDesc();
			TextureDesc desc{
				.Width = (uint32)resourceDesc.Width,
				.Height = (uint32)resourceDesc.Height,
				.Format = ResourceFormat::Unknown,
				.ClearColor = Color(1.0, 0.0, 0.0, 0.0)
			};
			auto pTexture = makeUnique<TextureDx12>(parent(), desc, pResource);
			pTexture->setSRV(static_cast<DeviceDx12*>(parent())->createSRV(pTexture.get(), TextureSRVDesc(0, 1)));
			backBuffers_.push_back(std::move(pTexture));
		}
	}

	void SwapChainDx12::resize(int width, int height)
	{
		nFence_->cpuWait();

		for (size_t i = 0; i < backBuffers_.size(); ++i)
		{
			backBuffers_[i]->setImmediateDelete();
			backBuffers_[i].reset();
		}
		backBuffers_.clear();
		currentBackBuffer_ = 0;
		DXGI_SWAP_CHAIN_DESC desc{};
		swapChain_->GetDesc(&desc);

		swapChain_->ResizeBuffers(
			(uint32)numFrames_,
			width,
			height,
			convertFormat(format_),
			desc.Flags
		);
		for (uint32 i = 0; i < numFrames_; ++i)
		{
			ID3D12ResourceX* pResource = nullptr;
			ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&pResource)));
			D3D12_RESOURCE_DESC resourceDesc = pResource->GetDesc();
			TextureDesc desc{
				.Width = (uint32)resourceDesc.Width,
				.Height = (uint32)resourceDesc.Height,
				.Format = ResourceFormat::Unknown,
				.ClearColor = Color(1.0, 0.0, 0.0, 0.0)
			};
			auto pTexture = makeUnique<TextureDx12>(parent(), desc, pResource);
			pTexture->setSRV(static_cast<DeviceDx12*>(parent())->createSRV(pTexture.get(), TextureSRVDesc(0, 1)));
			backBuffers_.push_back(std::move(pTexture));
		}
	}

	void SwapChainDx12::present()
	{
		swapChain_->Present(0, 0);

		// Signal and store when the GPU work for the frame we just flipped is finished.
		//CommandQueue* pDirectQueue = GetParent()->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
		//m_pPresentFence->Signal(pDirectQueue);
		curretntFence_++;

		// Add an instruction to the command queue to set a new fence point.  Because we 
		// are on the GPU timeline, the new fence point won't be set until the GPU finishes
		// processing all the commands prior to this Signal().
		
		CommandQueueDx12* rawCommandQueue = static_cast<CommandQueueDx12*>(parent()->commandQueue(CommandQueue::GRAPHICS));		
		nFence_->signal(rawCommandQueue);
		nFence_->cpuWait();

		// Wait for the next frame to be finished.
		currentBackBuffer_ = (currentBackBuffer_ + 1) % backBuffers_.size();
	}

	Texture* SwapChainDx12::backBuffer(int index) const 
	{
		return backBuffers_[index].get();
	}

	int SwapChainDx12::currentBackbuffer() const
	{
		return currentBackBuffer_;
	}

}