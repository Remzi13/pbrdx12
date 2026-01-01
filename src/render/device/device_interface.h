#pragma once

#include "core/std_types.h"
#include "core/memory.h"
#include "core/math_utils.h"

// TODO : remove this include 
#include "render/device/texture.h"
#include "render/device/allocator.h"

#include "render/formats.h"

#include "dxc/dxcapi.h"
#include "d3dx12/d3dx12.h"
#include "d3dx12/d3dx12_root_signature.h"
#include "d3dx12/d3dx12_core.h"


namespace render {

	class GraphicsDevice;
	class CommandQueue;
	class CommandContext;
	class Texture;
	class ResourceView;

	namespace device
	{
		class Fence;
	}	

	enum RootSignatureType
	{
		COMMON, 
		cout
	};

	enum class RenderPassColorFlags : uint8
	{
		None = 0,
		Clear = 1 << 0,
		Resolve = 1 << 1,
	};

	enum class RenderPassDepthFlags : uint8
	{
		None,
		ClearDepth = 1 << 0,
		ClearStencil = 1 << 1,
		ReadOnlyDepth = 1 << 2,
		ReadOnlyStencil = 1 << 3,

		ReadOnly = ReadOnlyDepth | ReadOnlyStencil,
		Clear = ClearDepth | ClearStencil,
	};

	struct BindingSlot
	{
		static constexpr uint32 PerInstance = 0;
		static constexpr uint32 PerPass = 1;
		static constexpr uint32 PerView = 2;
		static constexpr uint32 UAV = 3;
		static constexpr uint32 SRV = 4;
	};
		
	class SyncPoint
	{
	public:
		SyncPoint() = default;
		SyncPoint(device::Fence* fence, uint64 value)
			: fence_(fence), value_(value)
		{}

		void wait() const;
		bool isComplete() const;// { return fence_->isComplete(value_); }
		uint64 value() const { return value_; }
		device::Fence* fence() const { return fence_; }
		bool isValid() const { return !!fence_; }
		operator bool() const { return isValid(); }

	private:
		device::Fence* fence_ = nullptr;
		uint64 value_ = 0;
	};
	
	class DeviceObject
	{
	public:
		DeviceObject(GraphicsDevice* parent)
			: parent_(parent)
		{}
		virtual ~DeviceObject() = default;
			
		GraphicsDevice* parent() const { return parent_; }

	private:
		class GraphicsDevice* parent_;
	};

	class RootSignature : public DeviceObject
	{
	public:
		static constexpr int MaxNumParameters = 8;

	public:
		RootSignature(GraphicsDevice* parent) : DeviceObject(parent) {}

	};
	SharedPtr<RootSignature> getRootSignature(RootSignatureType type, GraphicsDevice* parent);

	class PipelineState : public DeviceObject
	{
	public:
		PipelineState(GraphicsDevice* parent) : DeviceObject(parent) {}		
		virtual void init() =0;
	};

	class Buffer
	{
	public:
		struct Desc
		{
			uint64			Size = 0;
			uint32			ElementSize = 1;
			BufferFlag		Flags = BufferFlag::None;
			ResourceFormat	Format = ResourceFormat::Unknown;

			[[nodiscard]] uint32 numElements() const { return static_cast<uint32>(Size / ElementSize); }
		};

		struct VertexView
		{
			//D3D12_GPU_VIRTUAL_ADDRESS Location;
			uint64 Location;
			uint32 Elements;
			uint32 Stride;
			uint32 OffsetFromStart;
		};

		struct IndexView
		{
			//D3D12_GPU_VIRTUAL_ADDRESS Location;
			uint64 Location;
			uint32 Elements;
			uint32 OffsetFromStart;
			ResourceFormat Format;
		};

		explicit Buffer(const Desc& desc) : desc_(desc) {}

		[[nodiscard]] uint64 size() const { return desc_.Size; }
		[[nodiscard]] void* mappedData() const { return mappedData_; }
		void setMappedData(void* data) { mappedData_ = data; }

		[[nodiscard]] Desc desc() const { return desc_; }

	private:
		Desc desc_;
		void* mappedData_{ nullptr };
	};

	class CommandQueue : public DeviceObject
	{
	public: 
		enum Type
		{
			GRAPHICS, //D3D12_COMMAND_LIST_TYPE_DIRECT
			COPY, 
			Count
		};

		CommandQueue(GraphicsDevice* device, Type type) : DeviceObject(device), type_(type) {}

		~CommandQueue() override = default;

		[[nodiscard]] virtual SyncPoint execute(CommandContext* context) = 0;

	private:
		Type type_;
	};

	class SwapChain : public DeviceObject
	{
	public: 
		SwapChain(GraphicsDevice* device, int numFrames) : DeviceObject(device), numFrames_(numFrames) {}

		virtual void resize(int width, int height) = 0;
		[[nodiscard]] virtual Texture* backBuffer(int index) const =0;
		[[nodiscard]] virtual int currentBackbuffer() const = 0;
		virtual void present() = 0;

	protected:
		int numFrames_;
	};


	class CommandContext : public DeviceObject
	{
	public: 
		
		struct RenderPassInfo
		{
			struct RenderTarget
			{
				Texture* texture{ nullptr };
				uint8 mipLevel = 0;
				RenderPassColorFlags Flags = RenderPassColorFlags::None;
			} renderTarget;

			struct DepthTarget
			{
				Texture* texture{ nullptr };
				uint8 mipLevel = 0;
				RenderPassDepthFlags Flags = RenderPassDepthFlags::None;
				struct Data
				{
					Data(float depth = 0.0f, uint8 stencil = 1)
						: Depth(depth), Stencil(stencil)
					{
					}
					float Depth;
					uint8 Stencil;
				} data;
				
			} depthTarget;


			RenderPassInfo() = default;
			RenderPassInfo(Texture* target, Texture* depthStencil, RenderPassColorFlags colorFlag = RenderPassColorFlags::Clear)
				: renderTarget({ target, 0, colorFlag })
			{
				if (depthStencil)
				{
					depthTarget.texture = depthStencil;
					depthTarget.Flags = RenderPassDepthFlags::Clear;
					depthTarget.data.Depth = depthStencil->clearColor().r;
					depthTarget.data.Stencil = static_cast<uint8>(depthStencil->clearColor().g);
				}
			}
		};		
		CommandContext(GraphicsDevice* device, CommandQueue::Type type) : DeviceObject(device), type_(type) {}
		virtual ~CommandContext() {}

		virtual void begin(RenderPassInfo info) = 0;
		virtual void end() = 0;
		virtual void reset() = 0;
		virtual void flushResourceBarriers() = 0;
		virtual void insertResourceBarrier(Texture* pResource, ResourceState beforeState, ResourceState afterState, uint32 subResource) = 0;
		virtual void copyBuffer(const Buffer* pSource, const Buffer* pTarget, uint64 size, uint64 sourceOffset, uint64 destinationOffset) = 0;
		virtual void addResourceBarrier(Texture* pResource, ResourceState beforeState, ResourceState afterState, uint32 subResource) = 0;
		virtual void setRootSignature(const RootSignature* pRootSignature) = 0;
		virtual void setPipelineState(const PipelineState* pipelineState) = 0;
		virtual void setPrimitiveTopology(const PrimitiveTopology topology) = 0;
		virtual void setViewport(const core::RectF& rect, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
		virtual device::Allocation allocate(uint64 size, uint32 alignment = 16u) = 0;
		virtual void setVertexBuffer(Buffer::VertexView view) = 0;
		virtual void setIndexBuffer(Buffer::IndexView view) = 0;
		virtual void setScissorRect(const core::RectF& rect) = 0;
		virtual void bindRootCBV(uint32 rootIndex, const void* data, uint32 size) = 0;
		virtual void bindResources(uint32 rootIndex, const Buffer* pViews, uint32 offset = 0) = 0;
		virtual SyncPoint execute() = 0;

		virtual void drawIndexedInstanced(uint32 indexCount, uint32 indexStart, uint32 instanceCount, uint32 minVertex = 0, uint32 instanceStart = 0) = 0;

		virtual void free(SyncPoint syncPoint) = 0;

		CommandQueue::Type type() const { return type_; }
		bool isComplete() const { return syncPoint_.isComplete(); }

	protected:
		CommandQueue::Type type_;
		SyncPoint syncPoint_;
	};
	
	class GraphicsDevice
	{
	public:
		virtual bool init(HWND hwnd) = 0;
		virtual CommandQueue* commandQueue(CommandQueue::Type type) = 0;
		virtual CommandContext* getCommandContext(CommandQueue::Type type) = 0;
		virtual void backCommandContext( CommandContext* context ) = 0;

		virtual std::shared_ptr<Texture> createTexture( const TextureDesc& desc, const char* name ) = 0;
		virtual UniquePtr<Buffer> createBuffer(const Buffer::Desc& desc, const char* name, const void* pInitData) = 0;

		virtual void tick() = 0;
		virtual void fini() = 0;
	};


	UniquePtr<GraphicsDevice> createDevice();
	UniquePtr<SwapChain> createSwapChain(GraphicsDevice* device, HWND hwnd, int width, int height, int numFrames, ResourceFormat format);
}