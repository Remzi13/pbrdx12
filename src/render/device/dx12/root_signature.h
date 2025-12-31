#pragma once

#include "core/std_types.h"
#include "render/device/dx12/dx12.h"
#include "render/device/device_interface.h"

namespace render {
	class RootSignatureDx12 : public RootSignature
	{
	public:
		RootSignatureDx12(GraphicsDevice* parent);
		//~RootSignatureDx12(); // Destructor for cleanup

		void addRootCBV(uint32 shaderRegister, uint32 space, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void addDescriptorTable(uint32 shaderRegister, uint32 numDescriptors, D3D12_DESCRIPTOR_RANGE_TYPE type, uint32 space, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void addStaticSampler(uint32 registerSlot, uint32 space, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE wrapMode, D3D12_COMPARISON_FUNC compareFunc = D3D12_COMPARISON_FUNC_ALWAYS);

		void finalize(const char* pName, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

		bool isRootConstant(uint32 index) const;
		uint32 numRootConstants(uint32 rootIndex) const;

		ID3D12RootSignature* raw() const { return rootSignature_.Get(); }

	private:
		uint32 getDWORDSize() const;

	private:
		vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers_;

		struct RootParameter
		{
			CD3DX12_ROOT_PARAMETER1 Data;
			CD3DX12_DESCRIPTOR_RANGE1 Range;
		};
		array<RootParameter, MaxNumParameters> rootParameters_{};
		int numParameters_{ 0 };
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	};
	
}

