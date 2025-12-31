#pragma once

#include "render/device/dx12/resource.h"
#include "render/device/dx12/ringbuffer_allocator.h"
#include "render/device/dx12/page_allocator.h"

#include <d3d12.h>
#include <dxgi1_6.h>

namespace render {

	class CPUDescriptorHeap;
	class GPUDescriptorHeap;
	class CommandQueueDx12;
	namespace device {
		class Fence;
	}

	class DeviceDx12 : public GraphicsDevice
	{
	public:
		bool init(HWND hwnd) override;
		void fini() override;
		void tick() override;

		CommandQueue* commandQueue(CommandQueue::Type type) override;
		
		CommandContext* getCommandContext(CommandQueue::Type type) override;
		void backCommandContext(CommandContext* context) override;
		SharedPtr<Texture> createTexture( const TextureDesc& desc, const char* name ) override;
		UniquePtr<Buffer> createBuffer(const Buffer::Desc& desc, const char* name, const void* pInitData) override;

		UniquePtr<Buffer> createBuffer(const Buffer::Desc& desc, ID3D12Heap* pHeap, uint64 offset, const char* pName, const void* pInitData = nullptr);

		SharedPtr<ShaderResourceView> createSRV(Texture* pTexture, const TextureSRVDesc& desc);
		SharedPtr<ShaderResourceView> createSRV(Buffer* pBuffer, const BufferSRVDesc& desc);

		IDXGIFactory4* factory() const { return factory_; }
		ID3D12Device5* device() const { return device_; }

		GPUDescriptorHeap* globalViewHeap() const { return GPUResourceViewHeap_.get(); }
		GPUDescriptorHeap* globalSamplerHeap() const { return GPUSamplerHeap_.get(); }

		void deferReleaseObject( ID3D12Object* pObject );

		void freeCpuDescriptor( D3D12_CPU_DESCRIPTOR_HANDLE descriptor );

		SharedPtr<Texture> createTexture(const TextureDesc& desc, const char* name, ID3D12Heap* pHeap, const vector<D3D12_SUBRESOURCE_DATA>& initData);

	private:
		DescriptorHandle registerGlobalResourceView(D3D12_CPU_DESCRIPTOR_HANDLE view);
		D3D12_CPU_DESCRIPTOR_HANDLE allocateCPUDescriptor();
		void idleGpu();
		

	private:
		IDXGIFactory4* factory_;
		ID3D12DeviceX* device_;
		UniquePtr<CPUDescriptorHeap> CPUResourceViewHeap_;
		UniquePtr<GPUDescriptorHeap> GPUResourceViewHeap_;
		UniquePtr<GPUDescriptorHeap> GPUSamplerHeap_;
		UniquePtr<CommandQueueDx12> graphicsQueue_;
		UniquePtr<CommandQueueDx12> copyQueue_;

		UniquePtr<device::PageAllocator> pageAllocator_;
		UniquePtr<device::RingBufferAllocator> ringBufferAllocator_;

		class DeleteQueue
		{
		private:
			struct FencedObject
			{
				device::Fence* pFence;
				uint64 FenceValue;
				ID3D12Object* pResource;
			};

		public:	
			void push( ID3D12Object* pResource, device::Fence* pFence );
			void clean();

		private:
			//std::mutex m_QueueCS;
			queue<FencedObject> queue_;
		};
		DeleteQueue deleteQueue_;

		UniquePtr<device::Fence> frameFance_;
		static const uint32 NUM_BUFFERS = 2;
		array<uint64, NUM_BUFFERS> frameValues_{};
		uint32 frameIndex_ = 0;

		array<queue<CommandContext*>, CommandQueue::Count> freeCommandContext_;
	};

}
