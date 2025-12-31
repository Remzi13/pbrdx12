#pragma once

#include "math/matrix.h"

#include "data/color.h"

#include "render/device/dx12/dx12.h"
#include "render/formats.h"

#include <DirectXMath.h>
#include <Windows.h>
#include <string>

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)						                                            \
{						                                                                    \
    HRESULT hr__ = (x);																		\
    std::wstring wfn = elm::render::device::AnsiToWString(__FILE__);						\
    if(FAILED(hr__)) { throw elm::render::device::DxException(hr__, L#x, wfn, __LINE__); }	\
}
#endif


namespace elm::render {

	DXGI_FORMAT convertFormat(ResourceFormat format);
	ResourceFormat convertFormat(DXGI_FORMAT format);
	D3D12_RESOURCE_STATES convertFormat(ResourceState state);
	ResourceState convertFormat(D3D12_RESOURCE_STATES state);

	void convertFormat(Color color, FLOAT outColor[4]);
	D3D12_PRIMITIVE_TOPOLOGY convertFormat(PrimitiveTopology topology);

	struct FormatInfo
	{
		const char* name;
		ResourceFormat format;
		uint8  BytesPerBlock;
		uint8 BlockSize;
	};

	const FormatInfo& formatInfo(ResourceFormat format);
	uint64 rowPitch(ResourceFormat format, uint32 width, uint32 mipIndex = 0);
	uint64 slicePitch(ResourceFormat format, uint32 width, uint32 height, uint32 mipIndex = 0);

	namespace device {

		class DxException
		{
		public:
			DxException() = default;
			DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

			std::wstring ToString()const;

			HRESULT ErrorCode = S_OK;
			std::wstring FunctionName;
			std::wstring Filename;
			int LineNumber = -1;
		};

		std::wstring AnsiToWString(const std::string& str);

		inline DirectX::XMFLOAT3 convert(const math::Vector3& vector)
		{
			return DirectX::XMFLOAT3(vector.x, vector.y, vector.z);
		}

		inline math::Vector3 convert(DirectX::XMFLOAT3 v)
		{
			return { v.x, v.y, v.z };
		}

		inline DirectX::XMFLOAT4 convert(const math::Vector4& vector)
		{
			return DirectX::XMFLOAT4(vector.x, vector.y, vector.z, vector.w);
		}

		inline math::Vector4 convert(DirectX::XMFLOAT4 v)
		{
			return { v.x, v.y, v.z, v.w };
		}

		inline DirectX::XMMATRIX convert(const math::Matrix4x4& matrix)
		{
			auto mat = DirectX::XMFLOAT4X4(matrix.data());
			return XMLoadFloat4x4(&mat);
		}

		inline math::Matrix4x4 convert(const DirectX::XMFLOAT4X4& matrix)
		{
			math::Matrix4x4 m;
			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
				{
					m[i][j] = matrix(i, j);
				}
			}
			return m;
		}


		math::Matrix4x4 perspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ);
		math::Matrix4x4 rotationAxis(const math::Vector3& axis, float angle);
		math::Matrix4x4 rotateY(float angle);
		bool triangleIntersect(const math::Vector4& rayOrigin, const math::Vector4& rayDir, const math::Vector3& v0, const math::Vector3& v1, const math::Vector3& v2, float& t);
	}
}