#include "render/device/dx12/root_signature.h"

#include "core/debug.h"

#include "render/device/dx12/device.h"

namespace elm::render {
	namespace {
		static D3D12_DESCRIPTOR_RANGE_FLAGS sDefaultTableRangeFlags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		static D3D12_ROOT_DESCRIPTOR_FLAGS sDefaultRootDescriptorFlags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

		array<SharedPtr<RootSignature>, RootSignatureType::cout> g_rootSignatures;

		SharedPtr<RootSignature> createRootSignature(GraphicsDevice* parent)
		{
			SharedPtr<RootSignatureDx12> rootSignature = makeShared<RootSignatureDx12>(parent);
			rootSignature->addRootCBV(0, 0);
			rootSignature->addRootCBV(1, 0);
			rootSignature->addRootCBV(2, 0);
			
			rootSignature->addDescriptorTable(0, 16, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0);
			rootSignature->addDescriptorTable(0, 64, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0);
			
			rootSignature->addStaticSampler(0, 1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
			rootSignature->addStaticSampler(1, 1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
			rootSignature->addStaticSampler(2, 1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER);
			rootSignature->addStaticSampler(3, 1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);


			rootSignature->finalize("Common Rootsignature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			return rootSignature;
		}
	}

	RootSignatureDx12::RootSignatureDx12(GraphicsDevice* parent) : RootSignature(parent)
	{

	}

	void RootSignatureDx12::addRootCBV(uint32 shaderRegister, uint32 space, D3D12_SHADER_VISIBILITY visibility)
	{
		RootParameter& parameter = rootParameters_[numParameters_++];
		parameter.Data.InitAsConstantBufferView(shaderRegister, space, sDefaultRootDescriptorFlags, visibility);
	}

	void RootSignatureDx12::addDescriptorTable(uint32 shaderRegister, uint32 numDescriptors, D3D12_DESCRIPTOR_RANGE_TYPE type, uint32 space, D3D12_SHADER_VISIBILITY visibility)
	{
		RootParameter& parameter = rootParameters_[numParameters_++];
		parameter.Range.Init(type, numDescriptors, shaderRegister, space, sDefaultTableRangeFlags, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
		parameter.Data.InitAsDescriptorTable(1, &parameter.Range, visibility);		
	}

	void RootSignatureDx12::addStaticSampler(uint32 registerSlot, uint32 space, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE wrapMode, D3D12_COMPARISON_FUNC compareFunc)
	{
		D3D12_STATIC_SAMPLER_DESC desc{
			.Filter = filter,
			.AddressU = wrapMode,
			.AddressV = wrapMode,
			.AddressW = wrapMode,
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 8,
			.ComparisonFunc = compareFunc,
			.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
			.MinLOD = 0.0f,
			.MaxLOD = FLT_MAX,
			.ShaderRegister = registerSlot,
			.RegisterSpace = space,
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
		};
		staticSamplers_.push_back(desc);
	}

	void RootSignatureDx12::finalize(const char* pName, D3D12_ROOT_SIGNATURE_FLAGS flags)
	{

		D3D12_ROOT_SIGNATURE_FLAGS visibilityFlags =
			D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		// TODO-MESHSHADING
		//if (!GetParent()->GetCapabilities().SupportsMeshShading())
		//{
		//	visibilityFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
		//	visibilityFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
		//}
		array<D3D12_ROOT_PARAMETER1, MaxNumParameters> rootParameters;
		for (size_t i = 0; i < numParameters_; ++i)
		{
			RootParameter& rootParameter = rootParameters_[i];
			switch (rootParameter.Data.ShaderVisibility)
			{
			case D3D12_SHADER_VISIBILITY_VERTEX:		visibilityFlags = visibilityFlags & ~D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;			break;
			case D3D12_SHADER_VISIBILITY_GEOMETRY:		visibilityFlags = visibilityFlags & ~D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;		break;
			case D3D12_SHADER_VISIBILITY_PIXEL:			visibilityFlags = visibilityFlags & ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;			break;
			case D3D12_SHADER_VISIBILITY_HULL:			visibilityFlags = visibilityFlags & ~D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;			break;
			case D3D12_SHADER_VISIBILITY_DOMAIN:		visibilityFlags = visibilityFlags & ~D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;			break;
			case D3D12_SHADER_VISIBILITY_MESH:			visibilityFlags = visibilityFlags & ~D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;			break;
			case D3D12_SHADER_VISIBILITY_AMPLIFICATION:	visibilityFlags = visibilityFlags & ~D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;	break;
			case D3D12_SHADER_VISIBILITY_ALL:			visibilityFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;														break;
			default:									ELM_ASSERT(false);																							break;
			}

			rootParameters[i] = rootParameter.Data;
		}

		if (!EnumHasAnyFlags(flags, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE))
		{
			flags |= visibilityFlags;
			flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
			flags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
		}

		constexpr uint32 recommendedDwords = 12;
		uint32 dwords = getDWORDSize();
		if (dwords > recommendedDwords)
		{
			//E_LOG(Warning, "[RootSignature::Finalize] RootSignature '%s' uses %d DWORDs while under %d is recommended", pName, dwords, recommendedDwords);
		}

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
		// BUG - problem with flags, if pass it brake D3D12SerializeVersionedRootSignature
		desc.Init_1_1(numParameters_, rootParameters.data(), (uint32)staticSamplers_.size(), staticSamplers_.data(), flags);
		
		ID3DBlob* pDataBlob;
		ID3DBlob* pErrorBlob;
		D3D12SerializeVersionedRootSignature(&desc, &pDataBlob, &pErrorBlob);		
		if (pErrorBlob)
		{
			const char* pError = (char*)pErrorBlob->GetBufferPointer();
			ELM_LOG(Error, "RootSignature serialization error: %s", pError);
			return;
		}
		ThrowIfFailed(static_cast<DeviceDx12*>(parent())->device()->CreateRootSignature(0, pDataBlob->GetBufferPointer(), pDataBlob->GetBufferSize(), IID_PPV_ARGS(rootSignature_.GetAddressOf())));		
		SetObjectName(rootSignature_.Get(), pName);
	}

	uint32 RootSignatureDx12::getDWORDSize() const
	{
		uint32 count = 0;
		for (size_t i = 0; i < numParameters_; ++i)
		{
			const RootParameter& rootParameter = rootParameters_[i];
			switch (rootParameter.Data.ParameterType)
			{
			case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
				count += rootParameter.Data.Constants.Num32BitValues;
				break;
			case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
				count += 1;
				break;
			case D3D12_ROOT_PARAMETER_TYPE_CBV:
			case D3D12_ROOT_PARAMETER_TYPE_SRV:
			case D3D12_ROOT_PARAMETER_TYPE_UAV:
				count += 2;
				break;
			}
		}
		return count;
	}

	uint32 RootSignatureDx12::numRootConstants(uint32 rootIndex) const
	{ 
		ELM_ASSERT(isRootConstant(rootIndex)); 
		return rootParameters_[rootIndex].Data.Constants.Num32BitValues;
	}

	bool RootSignatureDx12::isRootConstant(uint32 index) const
	{
		return rootParameters_[index].Data.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	}

	SharedPtr<RootSignature> getRootSignature(RootSignatureType type, GraphicsDevice* parent)
	{
		if (g_rootSignatures[type] == nullptr)
		{
			g_rootSignatures[type] = createRootSignature(parent);
		}
		return g_rootSignatures[type];
	}
}


