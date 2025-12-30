#include "render/device.h"

#include "core/debug.h"

#include "render/command_context.h"
#include "render/common.h"
#include "render/utils.h"

using Microsoft::WRL::ComPtr;

namespace render {

	bool Device::init()
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
		}
				
		if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,
			IID_PPV_ARGS(&factory_)))) {
			throw std::runtime_error("Failed to create DXGI Factory.");
		}

		ComPtr<IDXGIAdapter1> hardwareAdapter;
		for (UINT adapterIndex = 0;
			factory_->EnumAdapters1(adapterIndex, &hardwareAdapter) !=
			DXGI_ERROR_NOT_FOUND;
			++adapterIndex) {
			DXGI_ADAPTER_DESC1 desc;
			hardwareAdapter->GetDesc1(&desc);

			// Пропускаем программный адаптер Microsoft (это эмулятор)
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				continue;
			}


			if (FAILED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0,
				IID_PPV_ARGS(&device_)))) {
				throw std::runtime_error("Failed to create D3D12 Device.");
			}
		}

		if (FAILED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&device_)))) {
			throw std::runtime_error("Failed to create D3D12 Device.");
		}

		// --- 4. Создание "Очереди Команд" (Command Queue) ---	
		graphicsQueue_ = makeUnique<CommandQueue>(this, CommandQueue::Type::GRAPHICS);

		CPUResourceViewHeap_ = makeUnique<CPUDescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8196);
		GPUResourceViewHeap_ = makeUnique<GPUDescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, 1 << 18);

		constexpr uint32 KilobytesToBytes = 1 << 10;
		const uint64 scratchAllocatorPageSize = 256 * KilobytesToBytes;
		pageAllocator_ = makeUnique<PageAllocator>(this, BufferFlag::Upload, scratchAllocatorPageSize);

		constexpr uint32 MegaBytesToBytes = 1 << 20;
		const uint32 uploadRingBufferSize = 128 * MegaBytesToBytes;
		ringBufferAllocator_ = makeUnique<RingBufferAllocator>(this, uploadRingBufferSize);

		frameFance_ = makeUnique<Fence>(this, "FrameFence");

		return false;
	}

	void Device::tick()
	{
		deleteQueue_.clean();
		uint64 fenceValue = frameFance_->signal(graphicsQueue_.get());
		//pageAllocator_->free( SyncPoint( frameFance_.get(), fenceValue ) );
		frameValues_[frameIndex_ % NUM_BUFFERS] = fenceValue;
		++frameIndex_;
		frameFance_->cpuWait(frameValues_[frameIndex_ % NUM_BUFFERS]);
	}

	void Device::deferReleaseObject(ID3D12Object* object)
	{
		if (object)
		{
			deleteQueue_.push(object, frameFance_.get());
		}
	}

	CommandQueue* Device::commandQueue(CommandQueue::Type type) const
	{
		switch (type)
		{
		case CommandQueue::GRAPHICS: return graphicsQueue_.get();
		default:
			ASSERT(false);
		}
		return nullptr;
	}

	ID3D12Device* Device::device() const
	{
		return device_.Get();
	}

	IDXGIFactory4* Device::factory() const
	{
		return factory_.Get();
	}


	void Device::DeleteQueue::push(ID3D12Object* resource, Fence* fence)
	{
		FencedObject object;
		object.fence = fence;
		object.value = fence->currentValue();
		object.resource = resource;
		queue_.push(object);
	}

	void Device::DeleteQueue::clean()
	{
		// TODO : multithreading
		while (!queue_.empty())
		{
			const FencedObject& p = queue_.front();
			if (!p.fence->isComplete(p.value))
			{
				break;
			}
			CHECK(p.resource->Release(), == 0, "DeleteQueue");
			queue_.pop();
		}
	}

	SharedPtr<ShaderResourceView> Device::createSRV(Texture* pTexture, const TextureSRVDesc& desc)
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
			ASSERT(false);
		}


		D3D12_CPU_DESCRIPTOR_HANDLE descriptor = allocateCPUDescriptor();
		device_->CreateShaderResourceView(pTexture->resource(), &srvDesc, descriptor);
		DescriptorHandle gpuDescriptor = registerGlobalResourceView(descriptor);

		return makeShared<ShaderResourceView>(pTexture, descriptor, gpuDescriptor);
	}

	SharedPtr<ShaderResourceView> Device::createSRV(Buffer* pBuffer, const BufferSRVDesc& desc)
	{
		ASSERT(pBuffer);
		const Buffer::Desc& bufferDesc = pBuffer->desc();

		D3D12_CPU_DESCRIPTOR_HANDLE descriptor = allocateCPUDescriptor();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		if (enumHasAnyFlags(bufferDesc.Flags, BufferFlag::AccelerationStructure))
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.RaytracingAccelerationStructure.Location = pBuffer->gpuHandle();

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

			device_->CreateShaderResourceView(pBuffer->resource(), &srvDesc, descriptor);
		}


		DescriptorHandle gpuDescriptor;
		ASSERT(!enumHasAnyFlags(bufferDesc.Flags, BufferFlag::NoBindless));
		//if (!EnumHasAnyFlags(bufferDesc.Flags, BufferFlag::NoBindless))
		//	gpuDescriptor = RegisterGlobalResourceView(descriptor);

		return makeShared<ShaderResourceView>(pBuffer, descriptor, gpuDescriptor);
	}

	DescriptorHandle Device::registerGlobalResourceView(D3D12_CPU_DESCRIPTOR_HANDLE view)
	{
		DescriptorHandle handle = GPUResourceViewHeap_->allocatePersistent();
		device_->CopyDescriptorsSimple(1, handle.CpuHandle, view, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		return handle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Device::allocateCPUDescriptor()
	{
		return CPUResourceViewHeap_->allocateDescriptor();
	}

	SharedPtr<Texture> Device::createTexture(const TextureDesc& desc, const char* name)
	{
		return createTexture(desc, name, nullptr, {});
	}

	SharedPtr<Texture> Device::createTexture(const TextureDesc& textureDesc, const char* name, ID3D12Heap* pHeap, const vector<D3D12_SUBRESOURCE_DATA>& initData)
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
			ASSERT(false);
			break;
		}

		if (enumHasAnyFlags(textureDesc.Flags, TextureFlag::UnorderedAccess))
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (enumHasAnyFlags(textureDesc.Flags, TextureFlag::RenderTarget))
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (enumHasAnyFlags(textureDesc.Flags, TextureFlag::DepthStencil))
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			if (!enumHasAnyFlags(textureDesc.Flags, TextureFlag::ShaderResource))
			{
				//I think this can be a significant optimization on some devices because then the depth buffer can never be (de)compressed
				desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
			}
		}

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;
		ASSERT(enumHasAllFlags(textureDesc.Flags, TextureFlag::RenderTarget | TextureFlag::DepthStencil) == false);

		D3D12_CLEAR_VALUE* pClearValue = nullptr;
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = convertFormat(textureDesc.Format);

		if (enumHasAnyFlags(textureDesc.Flags, TextureFlag::RenderTarget))
		{
			//ELM_ASSERT(textureDesc.ClearColor == ClearBinding::ClearBindingValue::Color);
			memcpy(&clearValue.Color, &textureDesc.ClearColor, sizeof(Color));
			resourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			pClearValue = &clearValue;
		}
		if (enumHasAnyFlags(textureDesc.Flags, TextureFlag::DepthStencil))
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
			ASSERT(false);
			//uint64 offset;
			//ThrowIfFailed(device_->CreatePlacedResource(pHeap, offset, &desc, resourceState, pClearValue, IID_PPV_ARGS(&pResource)));
		}
		else
		{
			ThrowIfFailed(device_->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &desc, resourceState, pClearValue, IID_PPV_ARGS(&pResource)));
		}


		auto pTexture = makeShared<Texture>(this, textureDesc, pResource);
		pTexture->setResourceState(convertState(resourceState));
		pTexture->setName(name);

		if (initData.size() > 0)
		{
			uint64 requiredSize = 0;
			vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(initData.size());
			vector<uint32> numRows(initData.size());
			vector<uint64> rowSizes(initData.size());

			device_->GetCopyableFootprints(&desc, 0, initData.size(), 0, layouts.data(), numRows.data(), rowSizes.data(), &requiredSize);
			RingBufferAllocator::Allocation allocation;
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
				const CD3DX12_TEXTURE_COPY_LOCATION src(allocation.resource_->resource(), dstLayout);
				allocation.context_->list()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
			}

			ringBufferAllocator_->free(allocation);
		}

		if (enumHasAnyFlags(textureDesc.Flags, TextureFlag::ShaderResource))
		{
			// TODO : texture mipleves
			pTexture->setSRV(createSRV(pTexture.get(), TextureSRVDesc(0, 1)));
		}
		if (enumHasAnyFlags(textureDesc.Flags, TextureFlag::UnorderedAccess))
		{
			//pTexture->m_NeedsStateTracking = true;
			//
			//pTexture->m_UAVs.resize(desc.Mips);
			//for (uint8 mip = 0; mip < desc.Mips; ++mip)
			//	pTexture->m_UAVs[mip] = CreateUAV(pTexture, TextureUAVDesc(mip));
		}
		if (enumHasAnyFlags(textureDesc.Flags, TextureFlag::RenderTarget))
		{
			//ELM_ASSERT(false);
			//pTexture->m_NeedsStateTracking = true;
		}
		else if (enumHasAnyFlags(textureDesc.Flags, TextureFlag::DepthStencil))
		{
			//ELM_ASSERT(false);
			//pTexture->m_NeedsStateTracking = true;
		}

		return pTexture;
	}

	GPUDescriptorHeap* Device::globalViewHeap() const 
	{ 
		return GPUResourceViewHeap_.get(); 
	}

	GPUDescriptorHeap* Device::globalSamplerHeap() const
	{
		ASSERT(false); return nullptr; /*return GPUSamplerHeap_.get();*/ 
	}

	UniquePtr<Buffer> Device::createBuffer(const Buffer::Desc& desc, ID3D12Heap* pHeap, uint64 offset, const char* name, const void* pInitData)
	{
		auto GetResourceDesc = [](const Buffer::Desc& bufferDesc)
			{
				D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferDesc.Size, D3D12_RESOURCE_FLAG_NONE);
				if (enumHasAnyFlags(bufferDesc.Flags, BufferFlag::UnorderedAccess))
					desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				if (enumHasAnyFlags(bufferDesc.Flags, BufferFlag::AccelerationStructure))
					desc.Flags |= D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE;
				return desc;
			};

		D3D12_RESOURCE_DESC resourceDesc = GetResourceDesc(desc);
		D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
		constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_UNKNOWN = (D3D12_RESOURCE_STATES)-1;
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_UNKNOWN;

		if (enumHasAnyFlags(desc.Flags, BufferFlag::Readback))
		{
			ASSERT(initialState == D3D12_RESOURCE_STATE_UNKNOWN);
			initialState = D3D12_RESOURCE_STATE_COPY_DEST;
			heapType = D3D12_HEAP_TYPE_READBACK;
		}
		if (enumHasAnyFlags(desc.Flags, BufferFlag::Upload))
		{
			ASSERT(initialState == D3D12_RESOURCE_STATE_UNKNOWN);
			initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
			heapType = D3D12_HEAP_TYPE_UPLOAD;
		}
		if (enumHasAnyFlags(desc.Flags, BufferFlag::AccelerationStructure))
		{
			ASSERT(initialState == D3D12_RESOURCE_STATE_UNKNOWN);
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

		auto pBuffer = makeUnique<Buffer>(this, desc, pResource);
		pBuffer->setResourceState(initialState);
		pBuffer->setName(name);

		if (enumHasAnyFlags(desc.Flags, BufferFlag::Upload | BufferFlag::Readback))
		{
			void* data;
			ThrowIfFailed(pResource->Map(0, nullptr, &data));
			pBuffer->setMappedData(data);
			pBuffer->setNeedStateTracking(true);
		}

		bool isRaw = enumHasAnyFlags(desc.Flags, BufferFlag::ByteAddress);
		bool withCounter = !isRaw && desc.Format == ResourceFormat::Unknown;

		//#todo: Temp code. Pull out views from buffer
		if (enumHasAnyFlags(desc.Flags, BufferFlag::ShaderResource | BufferFlag::AccelerationStructure))
		{
			pBuffer->setSRV(createSRV(pBuffer.get(), BufferSRVDesc(desc.Format, isRaw)));
		}
		if (enumHasAnyFlags(desc.Flags, BufferFlag::UnorderedAccess))
		{
			ASSERT(false);
			//	pBuffer->m_pUAV = CreateUAV(pBuffer, BufferUAVDesc(desc.Format, isRaw, withCounter));
			//	pBuffer->m_NeedsStateTracking = true;
		}

		if (pInitData)
		{
			if (enumHasAllFlags(desc.Flags, BufferFlag::Upload))
			{
				memcpy((char*)pBuffer->mappedData(), pInitData, desc.Size);
			}
			else
			{
				RingBufferAllocator::Allocation allocation;
				ringBufferAllocator_->allocate((uint32)desc.Size, allocation);
				memcpy((char*)allocation.mappedMemory, pInitData, desc.Size);
				allocation.context_->copyBuffer(allocation.resource_.get(), pBuffer.get(), desc.Size, allocation.Offset, 0);
				ringBufferAllocator_->free(allocation);
			}
		}

		return pBuffer;
	}

	CommandContext* Device::getCommandContext(CommandQueue::Type type)
	{
		CommandContext* context = nullptr;
		if (!freeCommandContext_[type].empty() && freeCommandContext_[type].front()->isComplete())
		{
			context = freeCommandContext_[type].front();
			freeCommandContext_[type].pop();
		}
		else
		{
			context = new CommandContext(this, type, GPUResourceViewHeap_.get(), pageAllocator_.get());
		}
		context->reset();
		return context;
	}

}
