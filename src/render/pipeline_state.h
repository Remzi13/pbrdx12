#pragma once

#include "render/common.h"

#include <span>
#include <array>

namespace render {

	class Device;

	enum RootSignatureType
	{
		COMMON,
		cout
	};

	enum class ShaderType
	{
		Vertex,
		Pixel,
		COUNT,
	};


	class RootSignature : public DeviceObject
	{
	public:
		static constexpr int MaxNumParameters = 8;

	public:
		RootSignature(Device* parent) : DeviceObject(parent) {}

	};

	class PipelineStateDescriptor
	{
		friend class PipelineState;

	public:
		enum class BlendMode
		{
			Replace = 0,
			Alpha,
		};

		enum class DepthTest
		{
			Always = 0,
			Less,
			LessEqual
		};

		enum class CullMode
		{
			None = 0,
		};

		struct VertexElementDesc
		{
			const char* pSemantic;
			ResourceFormat Format;
			uint32 ByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			uint32 InputSlot = 0;
			uint32 InstanceStepRate = 0;
		};

		void setRootSignature(RootSignatureType type, Device* device);
		void setInputLayout(std::span<PipelineStateDescriptor::VertexElementDesc> layout);
		void setVertexShader(const char* path, const char* entryPoint);
		void setPixelShader(const char* path, const char* entryPoint);
		void setBlendMode(const BlendMode& blendMode);
		void setDepth(bool enabled, DepthTest test);
		void setRenderTargetFormat(ResourceFormat rtvFormat, ResourceFormat dsvFormat, uint32 msaa);
		void setCullMode(CullMode mode);

	private:
		vector<D3D12_INPUT_ELEMENT_DESC> inputElement_;
		D3D12_BLEND_DESC blendDesc_;
		D3D12_DEPTH_STENCIL_DESC dssDesc_;
		D3D12_RT_FORMAT_ARRAY rtFormats_;
		DXGI_SAMPLE_DESC sampleDesc_;
		DXGI_FORMAT dsvFormat_;
		CD3DX12_RASTERIZER_DESC rasterizer_;
		std::array<std::pair<string, string>, static_cast<size_t>(ShaderType::COUNT)> shaders_;
		memory::SharedPtr<RootSignature> rootSignature_;
	};

	class PipelineState : public DeviceObject
	{
	public:
		PipelineState(Device* device, const PipelineStateDescriptor& descriptor);

		void init();

		ID3D12PipelineState* raw() const { return pipelineState_.Get(); }

	private:
		PipelineStateDescriptor descriptor_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;
	};
}