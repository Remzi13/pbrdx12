#include "render/device/dx12/device.h"

#include "render/utils.h"
#include "render/device/dx12/texture.h"
#include "render/device/dx12/descriptorHeap.h"
#include "render/device/dx12/command_context.h"
#include "render/device/dx12/command_queue.h"
#include "render/device/dx12/swap_chain.h"
#include "render/device/dx12/fence.h"

extern "C" { _declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION; }
extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = ".\\"; }

namespace elm::render {

	class DeviceDx12;
	class CommandQueueDx12;	
	
	// ------------------------------------
	
	bool DeviceDx12::init(HWND hwnd)
	{
#if defined(DEBUG) || defined(_DEBUG)
		// Enable the D3D12 debug layer.
		{
			ID3D12Debug* debugController;
			ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			debugController->EnableDebugLayer();
		}
#endif

		ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory_)));

		// Try to create hardware device.
		// TODO : some errors, remove nullptr
		HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_));

		// Fallback to WARP device.
		if (FAILED(hardwareResult))
		{
			IDXGIAdapter* pWarpAdapter;
			ThrowIfFailed(factory_->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

			ThrowIfFailed(D3D12CreateDevice(pWarpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_)));
		}
		IDXGIAdapter1* adapter = nullptr;
		factory_->EnumAdapters1(0, &adapter);

		SetObjectName(device_, "Graphics Device");

		graphicsQueue_ = makeUnique<CommandQueueDx12>(this, CommandQueue::Type::GRAPHICS);
		copyQueue_ = makeUnique<CommandQueueDx12>(this, CommandQueue::Type::COPY);


		CPUResourceViewHeap_ = makeUnique<CPUDescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8196);
		GPUResourceViewHeap_ = makeUnique<GPUDescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, 1 << 18);
		GPUSamplerHeap_ = makeUnique<GPUDescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 32, 2048);

		constexpr uint32 KilobytesToBytes = 1 << 10;
		const uint64 scratchAllocatorPageSize = 256 * KilobytesToBytes;
		pageAllocator_ = makeUnique<device::PageAllocator>(this, BufferFlag::Upload, scratchAllocatorPageSize);

		constexpr uint32 MegaBytesToBytes = 1 << 20;
		const uint32 uploadRingBufferSize = 128 * MegaBytesToBytes;
		ringBufferAllocator_ = makeUnique<device::RingBufferAllocator>(this, uploadRingBufferSize);

		frameFance_ = std::make_unique<device::Fence>(this, "FrameFence");

		return true;
	}

	void DeviceDx12::tick()
	{
		deleteQueue_.clean();
		uint64 fenceValue = frameFance_->signal( graphicsQueue_.get() );
		//pageAllocator_->free( SyncPoint( frameFance_.get(), fenceValue ) );
		frameValues_[frameIndex_ %  NUM_BUFFERS] = fenceValue;
		++frameIndex_;
		frameFance_->cpuWait( frameValues_[frameIndex_ % NUM_BUFFERS] );
	}

	void DeviceDx12::fini()
	{
		for ( auto& contexts : freeCommandContext_ )
		{
			while (!contexts.empty())
			{

				delete contexts.front();
				contexts.pop();
			}
		}		

		pageAllocator_.reset();
		ringBufferAllocator_.reset();

		deleteQueue_.clean();

		CPUResourceViewHeap_.reset();
		GPUResourceViewHeap_.reset();
		//unique_ptr<GPUDescriptorHeap> GPUSamplerHeap_;
	}

	void DeviceDx12::idleGpu()
	{
		tick();
		frameFance_->cpuWait( frameFance_->lastCompleted() );
	}

	UniquePtr<Buffer> DeviceDx12::createBuffer(const Buffer::Desc& desc, const char* name, const void* pInitData)
	{
		return createBuffer(desc, nullptr, 0, name, pInitData);
	}

	UniquePtr<Buffer> DeviceDx12::createBuffer(const Buffer::Desc& desc, ID3D12Heap* pHeap, uint64 offset, const char* name, const void* pInitData)
	{
		auto GetResourceDesc = [](const Buffer::Desc& bufferDesc)
		{
				D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferDesc.Size, D3D12_RESOURCE_FLAG_NONE);
				if (EnumHasAnyFlags(bufferDesc.Flags, BufferFlag::UnorderedAccess))
					desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				if (EnumHasAnyFlags(bufferDesc.Flags, BufferFlag::AccelerationStructure))
					desc.Flags |= D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE;
				return desc;
		};
		
		D3D12_RESOURCE_DESC resourceDesc = GetResourceDesc(desc);
		D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
		constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_UNKNOWN = (D3D12_RESOURCE_STATES)-1;
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_UNKNOWN;
		
		if (EnumHasAnyFlags(desc.Flags, BufferFlag::Readback))
		{
			ELM_ASSERT(initialState == D3D12_RESOURCE_STATE_UNKNOWN);
			initialState = D3D12_RESOURCE_STATE_COPY_DEST;
			heapType = D3D12_HEAP_TYPE_READBACK;
		}
		if (EnumHasAnyFlags(desc.Flags, BufferFlag::Upload))
		{
			ELM_ASSERT(initialState == D3D12_RESOURCE_STATE_UNKNOWN);
			initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
			heapType = D3D12_HEAP_TYPE_UPLOAD;
		}
		if (EnumHasAnyFlags(desc.Flags, BufferFlag::AccelerationStructure))
		{
			ELM_ASSERT(initialState == D3D12_RESOURCE_STATE_UNKNOWN);
			initialState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		}
		
		if (initialState == D3D12_RESOURCE_STATE_UNKNOWN)
		{
			initialState = D3D12_RESOURCE_STATE_COMMON;
		}
		
		ID3D12ResourceX* pResource;
		D3D12_HEAP_PROPERTIES properties = CD3DX12_HEAP_PROPERTIES(heapType);
		
		if (pHeap)
		{
			ThrowIfFailed(device_->CreatePlacedResource(pHeap, offset, &resourceDesc, initialState, nullptr, IID_PPV_ARGS(&pResource)));
		}
		else
		{
			ThrowIfFailed(device_->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &resourceDesc, initialState, nullptr, IID_PPV_ARGS(&pResource)));
		}
		
		auto pBuffer = makeUnique<BufferDx12>(this, desc, pResource);
		pBuffer->setResourceState(initialState);
		pBuffer->setName(name);
		
		if (EnumHasAnyFlags(desc.Flags, BufferFlag::Upload | BufferFlag::Readback))
		{
			void* data;
			ThrowIfFailed(pResource->Map(0, nullptr, &data));
			pBuffer->setMappedData(data);
			pBuffer->setNeedStateTracking( true);
		}
		
		bool isRaw = EnumHasAnyFlags(desc.Flags, BufferFlag::ByteAddress);
		bool withCounter = !isRaw && desc.Format == ResourceFormat::Unknown;
		
		//#todo: Temp code. Pull out views from buffer
		if (EnumHasAnyFlags(desc.Flags, BufferFlag::ShaderResource | BufferFlag::AccelerationStructure))
		{
			pBuffer->setSRV( createSRV(pBuffer.get(), BufferSRVDesc(desc.Format, isRaw)));
		}
		if (EnumHasAnyFlags(desc.Flags, BufferFlag::UnorderedAccess))
		{
			ELM_ASSERT( false );
		//	pBuffer->m_pUAV = CreateUAV(pBuffer, BufferUAVDesc(desc.Format, isRaw, withCounter));
		//	pBuffer->m_NeedsStateTracking = true;
		}
		
		if (pInitData)
		{
			if (EnumHasAllFlags(desc.Flags, BufferFlag::Upload))
			{
				memcpy((char*)pBuffer->mappedData(), pInitData, desc.Size);
			}
			else
			{
				device::RingBufferAllocator::Allocation allocation;
				ringBufferAllocator_->allocate((uint32)desc.Size, allocation);
				memcpy((char*)allocation.mappedMemory, pInitData, desc.Size);
				allocation.context_->copyBuffer(allocation.resource_.get(), pBuffer.get(), desc.Size, allocation.Offset, 0);
				ringBufferAllocator_->free(allocation);
			}
		}
		
		return pBuffer;
	}


	DescriptorHandle DeviceDx12::registerGlobalResourceView(D3D12_CPU_DESCRIPTOR_HANDLE view)
	{
		DescriptorHandle handle = GPUResourceViewHeap_->allocatePersistent();
		device_->CopyDescriptorsSimple(1, handle.CpuHandle, view, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		return handle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DeviceDx12::allocateCPUDescriptor()
	{
		return CPUResourceViewHeap_->allocateDescriptor();
	}

	CommandQueue* DeviceDx12::commandQueue(CommandQueue::Type type) 
	{ 
		switch (type)
		{
		case elm::render::CommandQueue::GRAPHICS: return graphicsQueue_.get();
		case elm::render::CommandQueue::COPY: return copyQueue_.get();
		default:
			ELM_ASSERT(false);
		}
		return nullptr; 
	}

	SharedPtr<Texture> DeviceDx12::createTexture( const TextureDesc& desc, const char* name )
	{
		return createTexture( desc, name, nullptr, {} );
	}

	SharedPtr<Texture> DeviceDx12::createTexture(const TextureDesc& textureDesc,  const char* name, ID3D12Heap* pHeap, const vector<D3D12_SUBRESOURCE_DATA>& initData)
	{
		DXGI_FORMAT format = convertFormat(textureDesc.Format);

		D3D12_RESOURCE_DESC desc{};
		switch (textureDesc.Type)
		{
		
		case TextureType::Texture2D:
			desc = CD3DX12_RESOURCE_DESC::Tex2D(format, textureDesc.Width, textureDesc.Height, 1/*arraySize*/, textureDesc.Mips, textureDesc.SampleCount, 0, D3D12_RESOURCE_FLAG_NONE, D3D12_TEXTURE_LAYOUT_UNKNOWN);
			break;
		case TextureType::TextureCube:
			desc = CD3DX12_RESOURCE_DESC::Tex2D(format, textureDesc.Width, textureDesc.Height, 1/*arraySize*/ * 6, textureDesc.Mips, textureDesc.SampleCount, 0, D3D12_RESOURCE_FLAG_NONE, D3D12_TEXTURE_LAYOUT_UNKNOWN);
			break;
		default:
			ELM_ASSERT(false);
			break;
		}

		if (EnumHasAnyFlags(textureDesc.Flags, TextureFlag::UnorderedAccess))
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (EnumHasAnyFlags(textureDesc.Flags, TextureFlag::RenderTarget))
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (EnumHasAnyFlags(textureDesc.Flags, TextureFlag::DepthStencil))
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			if (!EnumHasAnyFlags(textureDesc.Flags, TextureFlag::ShaderResource))
			{
				//I think this can be a significant optimization on some devices because then the depth buffer can never be (de)compressed
				desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
			}
		}

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;		
		ELM_ASSERT(EnumHasAllFlags(textureDesc.Flags, TextureFlag::RenderTarget | TextureFlag::DepthStencil) == false);

		D3D12_CLEAR_VALUE* pClearValue = nullptr;
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = convertFormat(textureDesc.Format);

		if (EnumHasAnyFlags(textureDesc.Flags, TextureFlag::RenderTarget))
		{
			//ELM_ASSERT(textureDesc.ClearColor == ClearBinding::ClearBindingValue::Color);
			memcpy(&clearValue.Color, &textureDesc.ClearColor, sizeof(Color));
			resourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			pClearValue = &clearValue;
		}
		if (EnumHasAnyFlags(textureDesc.Flags, TextureFlag::DepthStencil))
		{
			//gAssert(desc.ClearBindingValue.BindingValue == ClearBinding::ClearBindingValue::DepthStencil);
			// TODO setup clear value 
			clearValue.DepthStencil.Depth = textureDesc.ClearColor.r; 
			clearValue.DepthStencil.Stencil = textureDesc.ClearColor.g;
			resourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			pClearValue = &clearValue;
		}		

		ID3D12ResourceX* pResource;
		D3D12_HEAP_PROPERTIES properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		
		
		if (pHeap)
		{
			ELM_ASSERT(false);
			//uint64 offset;
			//ThrowIfFailed(device_->CreatePlacedResource(pHeap, offset, &desc, resourceState, pClearValue, IID_PPV_ARGS(&pResource)));
		}
		else
		{
			ThrowIfFailed(device_->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &desc, resourceState, pClearValue, IID_PPV_ARGS(&pResource)));
		}


		auto pTexture = makeShared<TextureDx12>(this, textureDesc, pResource);
		pTexture->setResourceState(convertFormat(resourceState));
		pTexture->setName(name);

		if (initData.size() > 0)
		{
			uint64 requiredSize = 0;
			vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(initData.size());
			vector<uint32> numRows(initData.size());
			vector<uint64> rowSizes(initData.size());

			device_->GetCopyableFootprints(&desc, 0, initData.size(), 0, layouts.data(), numRows.data(), rowSizes.data(), &requiredSize);
			device::RingBufferAllocator::Allocation allocation;
			ringBufferAllocator_->allocate((uint32)requiredSize, allocation);

			for (uint32 subResource = 0; subResource < initData.size(); ++subResource)
			{
				const D3D12_SUBRESOURCE_DATA& srcData = initData[subResource];
				D3D12_PLACED_SUBRESOURCE_FOOTPRINT& dstLayout = layouts[subResource];

				D3D12_MEMCPY_DEST dest =
				{
					.pData = (char*)allocation.mappedMemory + dstLayout.Offset,
					.RowPitch = dstLayout.Footprint.RowPitch,
					.SlicePitch = (uint64)dstLayout.Footprint.RowPitch * numRows[subResource]
				};

				for (uint32 z = 0; z < dstLayout.Footprint.Depth; ++z)
				{
					char* pDest = (char*)dest.pData + dest.SlicePitch * z;
					const char* pSrc = (char*)srcData.pData + srcData.SlicePitch * z;
					for (uint32 y = 0; y < numRows[subResource]; ++y)
					{
						memcpy(pDest + y * dest.RowPitch, pSrc + y * srcData.RowPitch, rowSizes[subResource]);
					}
				}

				dstLayout.Offset += allocation.Offset;

				const CD3DX12_TEXTURE_COPY_LOCATION dst(pTexture->resource(), subResource);
				const CD3DX12_TEXTURE_COPY_LOCATION src(static_cast<BufferDx12*>(allocation.resource_.get())->resource(), dstLayout);
				static_cast<CommandContextDx12*>(allocation.context_)->list()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
			}

			ringBufferAllocator_->free(allocation);			
		}

		if (EnumHasAnyFlags(textureDesc.Flags, TextureFlag::ShaderResource))
		{
			// TODO : texture mipleves
			pTexture->setSRV( createSRV(pTexture.get(), TextureSRVDesc(0, 1)));
		}
		if (EnumHasAnyFlags(textureDesc.Flags, TextureFlag::UnorderedAccess))
		{
			//pTexture->m_NeedsStateTracking = true;
			//
			//pTexture->m_UAVs.resize(desc.Mips);
			//for (uint8 mip = 0; mip < desc.Mips; ++mip)
			//	pTexture->m_UAVs[mip] = CreateUAV(pTexture, TextureUAVDesc(mip));
		}
		if (EnumHasAnyFlags(textureDesc.Flags, TextureFlag::RenderTarget))
		{
			//ELM_ASSERT(false);
			//pTexture->m_NeedsStateTracking = true;
		}
		else if (EnumHasAnyFlags(textureDesc.Flags, TextureFlag::DepthStencil))
		{
			//ELM_ASSERT(false);
			//pTexture->m_NeedsStateTracking = true;
		}

		return pTexture;
	}

	SharedPtr<ShaderResourceView> DeviceDx12::createSRV(Texture* pTexture, const TextureSRVDesc& desc)
	{
		const TextureDesc& textureDesc = pTexture->desc();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		
		srvDesc.Format = convertFormat(textureDesc.Format);

		switch (textureDesc.Type)
		{
			case TextureType::Texture2D:
				srvDesc.Texture2D.MipLevels = desc.NumMipLevels;
				srvDesc.Texture2D.MostDetailedMip = desc.MipLevel;
				srvDesc.Texture2D.PlaneSlice = 0;
				srvDesc.Texture2D.ResourceMinLODClamp = 0;
				srvDesc.ViewDimension = textureDesc.SampleCount > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D;
				break;
			case TextureType::TextureCube:
				srvDesc.TextureCube.MipLevels = desc.NumMipLevels;
				srvDesc.TextureCube.MostDetailedMip = desc.MipLevel;
				srvDesc.TextureCube.ResourceMinLODClamp = 0;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				break;
			default:
				ELM_ASSERT(false);
		}


		D3D12_CPU_DESCRIPTOR_HANDLE descriptor = allocateCPUDescriptor();
		device_->CreateShaderResourceView(static_cast<TextureDx12*>(pTexture)->resource(), &srvDesc, descriptor);
		DescriptorHandle gpuDescriptor = registerGlobalResourceView(descriptor);
		
		return makeShared<ShaderResourceView>(static_cast<TextureDx12*>(pTexture), descriptor, gpuDescriptor);
	}

	SharedPtr<ShaderResourceView> DeviceDx12::createSRV(Buffer* pBuffer, const BufferSRVDesc& desc)
	{
		ELM_ASSERT(pBuffer);
		const Buffer::Desc& bufferDesc = pBuffer->desc();

		D3D12_CPU_DESCRIPTOR_HANDLE descriptor = allocateCPUDescriptor();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		if (EnumHasAnyFlags(bufferDesc.Flags, BufferFlag::AccelerationStructure))
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.RaytracingAccelerationStructure.Location = static_cast<BufferDx12*>(pBuffer)->gpuHandle();

			device_->CreateShaderResourceView(nullptr, &srvDesc, descriptor);
		}
		else
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			if (desc.Raw)
			{
				srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
				srvDesc.Buffer.StructureByteStride = 0;
				srvDesc.Buffer.FirstElement = desc.ElementOffset / 4;
				srvDesc.Buffer.NumElements = desc.NumElements > 0 ? desc.NumElements / 4 : (uint32)(bufferDesc.Size / 4);
				srvDesc.Buffer.Flags |= D3D12_BUFFER_SRV_FLAG_RAW;
			}

			else
			{
				srvDesc.Format = convertFormat(desc.Format);
				srvDesc.Buffer.StructureByteStride = desc.Format == ResourceFormat::Unknown ? bufferDesc.ElementSize : 0;
				srvDesc.Buffer.FirstElement = desc.ElementOffset;
				srvDesc.Buffer.NumElements = desc.NumElements > 0 ? desc.NumElements : bufferDesc.numElements();
			}

			device_->CreateShaderResourceView(static_cast<BufferDx12*>(pBuffer)->resource(), &srvDesc, descriptor);
		}

		
		DescriptorHandle gpuDescriptor;
		ELM_ASSERT(!EnumHasAnyFlags(bufferDesc.Flags, BufferFlag::NoBindless));
		//if (!EnumHasAnyFlags(bufferDesc.Flags, BufferFlag::NoBindless))
		//	gpuDescriptor = RegisterGlobalResourceView(descriptor);

		return makeShared<ShaderResourceView>(static_cast<BufferDx12*>(pBuffer), descriptor, gpuDescriptor);
	}

	CommandContext* DeviceDx12::getCommandContext(CommandQueue::Type type)
	{
		CommandContext* context = nullptr;
		if ( !freeCommandContext_[type].empty() && freeCommandContext_[type].front()->isComplete())
		{
			context = freeCommandContext_[type].front();
			freeCommandContext_[type].pop();
		}
		else
		{
			context = new CommandContextDx12( this, type, GPUResourceViewHeap_.get(), pageAllocator_.get() );
		}
		context->reset();
		return context;
	}

	void DeviceDx12::backCommandContext( CommandContext* context ) 
	{
		freeCommandContext_[context->type()].push( context );
	}

	void DeviceDx12::deferReleaseObject( ID3D12Object* object )
	{
		if ( object )
		{
			deleteQueue_.push( object, frameFance_.get() );
		}
	}

	void DeviceDx12::freeCpuDescriptor( D3D12_CPU_DESCRIPTOR_HANDLE descriptor )
	{
		CPUResourceViewHeap_->freeDescriptor( descriptor );
	}

	void DeviceDx12::DeleteQueue::push( ID3D12Object* resource, device::Fence* fence )
	{
		FencedObject object;
		object.pFence = fence;
		object.FenceValue = fence->currentValue();
		object.pResource = resource;
		queue_.push( object );
	}

	void DeviceDx12::DeleteQueue::clean()
	{
		// TODO : multithreading
		while ( !queue_.empty() )
		{
			const FencedObject& p = queue_.front();
			if ( !p.pFence->isComplete( p.FenceValue ) )
			{
				break;
			}
			ELM_CHECK( p.pResource->Release(), == 0, "DeleteQueue");
			queue_.pop();
		}
	}

	UniquePtr<GraphicsDevice> createDevice()
	{
		return makeUnique<DeviceDx12>();
	}

	UniquePtr<SwapChain> createSwapChain(GraphicsDevice* device, HWND hwnd, int width, int height, int numFrames, ResourceFormat format)
	{
		return makeUnique<SwapChainDx12>(static_cast<DeviceDx12*>(device), hwnd, width, height, numFrames, format);
	}


}