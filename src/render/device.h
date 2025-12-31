#pragma once 

#include "core/memory.h"


#include "render/command_queue.h"
#include "render/descriptor_heap.h"
#include "render/texture.h"
#include "render/ringbuffer_allocator.h"
#include "render/page_allocator.h"

using Microsoft::WRL::ComPtr;

namespace render {

	using namespace memory;

	class Device
	{
	public: 
		bool init();

		void tick();


		void deferReleaseObject(ID3D12Object* object);

		CommandQueue* commandQueue(CommandQueue::Type type) const;
		SharedPtr<ShaderResourceView> createSRV(Texture* pTexture, const TextureSRVDesc& desc);
		SharedPtr<ShaderResourceView> createSRV(Buffer* pBuffer, const BufferSRVDesc& desc);
		SharedPtr<Texture> createTexture(const TextureDesc& desc, const char* name);
		SharedPtr<Texture> createTexture(const TextureDesc& desc, const char* name, ID3D12Heap* pHeap, const vector<D3D12_SUBRESOURCE_DATA>& initData);
		UniquePtr<Buffer> createBuffer(const Buffer::Desc& desc, ID3D12Heap* pHeap, uint64 offset, const char* name, const void* pInitData);

		CommandContext* getCommandContext(CommandQueue::Type type);

		GPUDescriptorHeap* globalViewHeap() const;
		GPUDescriptorHeap* globalSamplerHeap() const;


		ID3D12Device* device() const;
		IDXGIFactory4* factory() const;

	private:		
		DescriptorHandle registerGlobalResourceView(D3D12_CPU_DESCRIPTOR_HANDLE view);
		D3D12_CPU_DESCRIPTOR_HANDLE allocateCPUDescriptor();

	private:
		ComPtr<ID3D12Device> device_;
		ComPtr<IDXGIFactory4> factory_;

		UniquePtr<CommandQueue> graphicsQueue_;

		UniquePtr<CPUDescriptorHeap> CPUResourceViewHeap_;
		UniquePtr<GPUDescriptorHeap> GPUResourceViewHeap_;

		UniquePtr<Fence> frameFance_;

		class DeleteQueue
		{
		private:
			struct FencedObject
			{
				Fence* fence;
				uint64 value;
				ID3D12Object* resource;
			};

		public:
			void push(ID3D12Object* pResource, Fence* pFence);
			void clean();

		private:
			//std::mutex m_QueueCS;
			queue<FencedObject> queue_;
		};
		DeleteQueue deleteQueue_;

		UniquePtr<RingBufferAllocator> ringBufferAllocator_;

		std::array<queue<CommandContext*>, CommandQueue::Count> freeCommandContext_;
		UniquePtr<PageAllocator> pageAllocator_;

		static const uint32 NUM_BUFFERS = 2;
		std::array<uint64, NUM_BUFFERS> frameValues_{};
		uint32 frameIndex_ = 0;
	};
}