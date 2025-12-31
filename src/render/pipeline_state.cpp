#include "render/pipeline_state.h"

#include "core/debug.h"

#include "render/device.h"
#include "render/utils.h"

namespace render {

	using namespace memory;

	SharedPtr<RootSignature> createRootSignature(Device* parent)
	{
		//SharedPtr<RootSignature> rootSignature = makeShared<RootSignature>(parent);
		//rootSignature->addRootCBV(0, 0);
		//rootSignature->addRootCBV(1, 0);
		//rootSignature->addRootCBV(2, 0);
		//
		//rootSignature->addDescriptorTable(0, 16, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0);
		//rootSignature->addDescriptorTable(0, 64, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0);
		//
		//rootSignature->addStaticSampler(0, 1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		//rootSignature->addStaticSampler(1, 1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		//rootSignature->addStaticSampler(2, 1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER);
		//rootSignature->addStaticSampler(3, 1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		//
		//
		//rootSignature->finalize("Common Rootsignature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		//
		//return rootSignature;
		return nullptr;
	}


	void PipelineStateDescriptor::setRootSignature(RootSignatureType type, Device* device)
	{
		rootSignature_ = createRootSignature(device);
	}

	void PipelineStateDescriptor::setInputLayout(std::span<VertexElementDesc> layout)
	{
		inputElement_.clear();
		for (const VertexElementDesc& element : layout)
		{
			D3D12_INPUT_ELEMENT_DESC desc;
			desc.AlignedByteOffset = element.ByteOffset;
			desc.Format = convertFormat(element.Format);
			desc.InputSlot = 0;
			desc.InputSlotClass = element.InstanceStepRate > 0 ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			desc.InstanceDataStepRate = element.InstanceStepRate;
			desc.SemanticIndex = 0;
			desc.SemanticName = element.pSemantic;
			inputElement_.push_back(desc);
		}
	}

	void PipelineStateDescriptor::setVertexShader(const char* path, const char* entryPoint)
	{
		shaders_[(int)ShaderType::Vertex] = std::make_pair<string, string>(path, entryPoint);
	}

	void PipelineStateDescriptor::setPixelShader(const char* path, const char* entryPoint)
	{
		shaders_[(int)ShaderType::Pixel] = std::make_pair<string, string>(path, entryPoint);
	}

	void PipelineStateDescriptor::setBlendMode(const BlendMode& blendMode)
	{
		blendDesc_ = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		D3D12_RENDER_TARGET_BLEND_DESC& desc = blendDesc_.RenderTarget[0];
		desc.RenderTargetWriteMask = 0xf;
		desc.BlendEnable = blendMode == BlendMode::Replace ? false : true;

		switch (blendMode)
		{
		case BlendMode::Replace:
			desc.SrcBlend = D3D12_BLEND_ONE;
			desc.DestBlend = D3D12_BLEND_ZERO;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			break;
		case BlendMode::Alpha:
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			break;
		default:
			ASSERT(false);
			break;
		}
	}

	void PipelineStateDescriptor::setDepth(bool enabled, DepthTest test)
	{
		dssDesc_ = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		dssDesc_.DepthWriteMask = enabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		switch (test)
		{
		case DepthTest::Always:
			dssDesc_.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			break;
		case DepthTest::Less:
			dssDesc_.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			break;
		case DepthTest::LessEqual:
			dssDesc_.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			break;
		default:
			ASSERT(false);
			break;
		}
	}

	void PipelineStateDescriptor::setRenderTargetFormat(ResourceFormat rtvFormat, ResourceFormat dsvFormat, uint32 msaa)
	{
		// Validation layer bug - Throws error about RT Format even if NumRenderTargets == 0.
		memset(rtFormats_.RTFormats, 0, sizeof(DXGI_FORMAT) * ARRAYSIZE(rtFormats_.RTFormats));
		rtFormats_.NumRenderTargets = 0;
		rtFormats_.RTFormats[0] = convertFormat(rtvFormat);

		sampleDesc_.Count = msaa;
		sampleDesc_.Quality = 0;

		rasterizer_ = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		rasterizer_.MultisampleEnable = msaa > 1;

		dsvFormat_ = convertFormat(dsvFormat);
	}

	void PipelineStateDescriptor::setCullMode(CullMode mode)
	{
		switch (mode)
		{
		case CullMode::None:
			rasterizer_.CullMode = D3D12_CULL_MODE_NONE;
			break;
		default:
			ASSERT(false);
			break;
		}
	}

	void PipelineState::init()
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
		ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		psoDesc.InputLayout = { descriptor_.inputElement_.data(), (UINT)descriptor_.inputElement_.size() };
		//psoDesc.pRootSignature = descriptor_.rootSignature_->raw();
		//auto ps = device::ShaderManager::getShader(descriptor_.shaders_[(int)ShaderType::Pixel].first.c_str(), descriptor_.shaders_[(int)ShaderType::Pixel].second.c_str(), "ps_6_6")->blob;
		//auto vs = device::ShaderManager::getShader(descriptor_.shaders_[(int)ShaderType::Vertex].first.c_str(), descriptor_.shaders_[(int)ShaderType::Vertex].second.c_str(), "vs_6_6")->blob;
		//psoDesc.VS =
		//{
		//	reinterpret_cast<BYTE*>(vs->GetBufferPointer()),
		//	vs->GetBufferSize()
		//};
		//psoDesc.PS =
		//psoDesc.PS =
		//{
		//{
		//	reinterpret_cast<BYTE*>(ps->GetBufferPointer()),
		//	reinterpret_cast<BYTE*>(ps->GetBufferPointer()),
		//	ps->GetBufferSize()
		//	ps->GetBufferSize()
		//};
		//};
		//
		//
		//psoDesc.RasterizerState = descriptor_.rasterizer_;
		//psoDesc.RasterizerState = descriptor_.rasterizer_;
		//psoDesc.BlendState = descriptor_.blendDesc_;
		//psoDesc.BlendState = descriptor_.blendDesc_;
		//psoDesc.DepthStencilState = descriptor_.dssDesc_;
		//psoDesc.DepthStencilState = descriptor_.dssDesc_;
		//psoDesc.DepthStencilState = descriptor_.dssDesc_;
		//psoDesc.DepthStencilState = descriptor_.dssDesc_;
		//psoDesc.NumRenderTargets = 1;
		//psoDesc.NumRenderTargets = 1;
		//psoDesc.RTVFormats[0] = descriptor_.rtFormats_.RTFormats[0];
		//psoDesc.RTVFormats[0] = descriptor_.rtFormats_.RTFormats[0];
		//psoDesc.SampleDesc = descriptor_.sampleDesc_;
		//psoDesc.SampleDesc = descriptor_.sampleDesc_;
		//psoDesc.DSVFormat = descriptor_.dsvFormat_;
		//psoDesc.DSVFormat = descriptor_.dsvFormat_;
		//
		//
		//psoDesc.SampleMask = UINT_MAX;
		//psoDesc.SampleMask = UINT_MAX;
		//psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		//psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		ThrowIfFailed(parent()->device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_)));
	}



}