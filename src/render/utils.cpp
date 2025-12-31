#include "render/utils.h"

#include "core/debug.h"

#include "math/utils.h"

#include <DirectXCollision.h>

#include <comdef.h>


namespace elm::render
{

	namespace {
#define FORMAT_TYPE(name) #name, ResourceFormat::name

		constexpr FormatInfo g_formatInfo[] = {
			{FORMAT_TYPE(Unknown),		0,	0}, 
			{FORMAT_TYPE(R16_UINT),		2,	1},
			{FORMAT_TYPE(R32_UINT),		4,	1},
			{FORMAT_TYPE(RG32_FLOAT),	8,	1},
			{FORMAT_TYPE(RGB32_FLOAT),	12, 1},
			{FORMAT_TYPE(RGBA8_UNORM),	4,	1},
			{FORMAT_TYPE(RGBA32_FLOAT),	16, 1},
			{FORMAT_TYPE(D24S8),		4,	1},
			{FORMAT_TYPE(BGRA8_UNORM),	4,	1},
			{FORMAT_TYPE(BC1_UNORM),	8,	4},
			{FORMAT_TYPE(BC2_UNORM),	16, 4},
			{FORMAT_TYPE(BC3_UNORM),	16,	4},
			{FORMAT_TYPE(BC4_UNORM),	8,	4},
			{FORMAT_TYPE(BC5_UNORM),	16,	4}
		};
		static_assert(ARRAYSIZE(g_formatInfo) == static_cast<uint32>(ResourceFormat::Count));


	}

	DXGI_FORMAT convertFormat(ResourceFormat format)
	{
		switch (format)
		{
		case ResourceFormat::Unknown:		return	DXGI_FORMAT_UNKNOWN;
		case ResourceFormat::RG32_FLOAT:	return DXGI_FORMAT_R32G32_FLOAT;
		case ResourceFormat::RGB32_FLOAT:	return DXGI_FORMAT_R32G32B32_FLOAT;
		case ResourceFormat::RGBA8_UNORM:	return DXGI_FORMAT_R8G8B8A8_UNORM;
		case ResourceFormat::R16_UINT:		return DXGI_FORMAT_R16_UINT;
		case ResourceFormat::R32_UINT:		return DXGI_FORMAT_R32_UINT;
		case ResourceFormat::RGBA32_FLOAT:	return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case ResourceFormat::D24S8:			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case ResourceFormat::BGRA8_UNORM:	return DXGI_FORMAT_B8G8R8A8_UNORM;
		case ResourceFormat::BC1_UNORM:		return DXGI_FORMAT_BC1_UNORM;
		case ResourceFormat::BC3_UNORM:		return DXGI_FORMAT_BC3_UNORM;
		case ResourceFormat::BC4_UNORM:		return DXGI_FORMAT_BC4_UNORM;
		default:
			ELM_ASSERT(false);
		}
		return	DXGI_FORMAT_UNKNOWN;
	}

	ResourceFormat convertFormat(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_UNKNOWN:				return ResourceFormat::Unknown;
		case DXGI_FORMAT_R32G32_FLOAT:			return ResourceFormat::RG32_FLOAT;
		case DXGI_FORMAT_R32G32B32_FLOAT:		return ResourceFormat::RGB32_FLOAT;
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return ResourceFormat::RGBA8_UNORM;
		case DXGI_FORMAT_R16_UINT:				return ResourceFormat::R16_UINT;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:	return ResourceFormat::RGBA32_FLOAT;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:		return ResourceFormat::D24S8;
		default:
			ELM_ASSERT(false);
		}
		return ResourceFormat::Unknown;
	}

	ResourceState convertFormat(D3D12_RESOURCE_STATES state)
	{
		if (state == D3D12_RESOURCE_STATE_PRESENT)
			return ResourceState::Present;
		switch (state)
		{
		case D3D12_RESOURCE_STATE_COMMON: return ResourceState::Common;
		case D3D12_RESOURCE_STATE_RENDER_TARGET: return ResourceState::RenderTarget;
		case D3D12_RESOURCE_STATE_DEPTH_WRITE: return ResourceState::Depth;		
		case D3D12_RESOURCE_STATE_DEPTH_READ:
		case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
		case D3D12_RESOURCE_STATE_INDEX_BUFFER:
		case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
		case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
		case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
		case D3D12_RESOURCE_STATE_STREAM_OUT:
		case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:
		case D3D12_RESOURCE_STATE_COPY_DEST:
		case D3D12_RESOURCE_STATE_COPY_SOURCE:
		case D3D12_RESOURCE_STATE_RESOLVE_DEST:
		case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:
		case D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE:
		case D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE:
		case D3D12_RESOURCE_STATE_RESERVED_INTERNAL_8000:
		case D3D12_RESOURCE_STATE_RESERVED_INTERNAL_4000:
		case D3D12_RESOURCE_STATE_RESERVED_INTERNAL_100000:
		case D3D12_RESOURCE_STATE_RESERVED_INTERNAL_40000000:
		case D3D12_RESOURCE_STATE_RESERVED_INTERNAL_80000000:
		case D3D12_RESOURCE_STATE_GENERIC_READ:
		case D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE:
		//case D3D12_RESOURCE_STATE_PREDICATION:
		case D3D12_RESOURCE_STATE_VIDEO_DECODE_READ:
		case D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE:
		case D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ:
		case D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE:
		case D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ:
		case D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE:
		default:
			ELM_ASSERT(false);
		}
		return ResourceState::Unknown;
	}

	D3D12_RESOURCE_STATES convertFormat(ResourceState format)
	{
		switch (format)
		{
		case elm::render::Common:		return D3D12_RESOURCE_STATE_COMMON;
		case elm::render::Unknown:		return D3D12_RESOURCE_STATE_COMMON;
		case elm::render::Present:		return D3D12_RESOURCE_STATE_PRESENT;
		case elm::render::RenderTarget:	return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case elm::render::Depth:		return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		default:
			ELM_ASSERT(false);
			break;
		}		
		return D3D12_RESOURCE_STATE_COMMON;
	}

	void convertFormat(Color color, FLOAT outColor[4])
	{
		outColor[0] = color.r;
		outColor[1] = color.g;
		outColor[2] = color.b;
		outColor[3] = color.a;
	}

	D3D12_PRIMITIVE_TOPOLOGY convertFormat(PrimitiveTopology topology)
	{
		switch (topology)
		{
		case elm::render::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		default:
			ELM_ASSERT(false);
			break;
		}
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}

	const FormatInfo& formatInfo(ResourceFormat format)
	{
		const FormatInfo& info = g_formatInfo[(uint32)format];
		ELM_ASSERT(info.format == format);
		return info;
	}

	uint64 rowPitch(ResourceFormat format, uint32 width, uint32 mipIndex)
	{
		const FormatInfo& info = formatInfo(format);
		if (info.BlockSize > 0)
		{
			uint64 numBlocks = math::utils::Max(1u, math::utils::divideAndRoundUp(width >> mipIndex, info.BlockSize));
			return numBlocks * info.BytesPerBlock;
		}
		return 0;
	}

	uint64 slicePitch(ResourceFormat format, uint32 width, uint32 height, uint32 mipIndex)
	{
		const FormatInfo& info = formatInfo(format);
		if (info.BlockSize > 0)
		{
			uint64 numBlocksX = math::utils::Max(1u, math::utils::divideAndRoundUp(width >> mipIndex, info.BlockSize));
			uint64 numBlocksY = math::utils::Max(1u, math::utils::divideAndRoundUp(height >> mipIndex, info.BlockSize));
			return numBlocksX * numBlocksY * info.BytesPerBlock;
		}
		return 0;
	}

}

namespace elm::render::device {

	using namespace DirectX;
	using namespace math;
		
	Matrix4x4 perspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ)
	{
		XMMATRIX P = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ);
		XMFLOAT4X4 proj;
		XMStoreFloat4x4(&proj, P);

		return convert(proj);
	}

	Matrix4x4 rotationAxis(const Vector3& axis, float angle)
	{
		XMFLOAT3 a = convert(axis);
		XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&a), angle);

		XMFLOAT4X4 r;
		XMStoreFloat4x4(&r, R);

		return convert(r);
	}

	Matrix4x4 rotateY(float angle)
	{
		XMMATRIX R = XMMatrixRotationY(angle);
		DirectX::XMFLOAT4X4 r;
		XMStoreFloat4x4(&r, R);

		return convert(r);
	}

	bool triangleIntersect(const math::Vector4& rayOrigin, const math::Vector4& rayDir, const math::Vector3& v0, const math::Vector3& v1, const math::Vector3& v2, float& t)
	{
		XMVECTOR origin = XMVectorSet(rayOrigin.x, rayOrigin.y, rayOrigin.z, rayOrigin.w);		
		XMVECTOR dir = XMVectorSet(rayDir.x, rayDir.y, rayDir.z, rayDir.w);
		auto tmp = convert(v0);
		XMVECTOR xv0 = XMLoadFloat3(&tmp);
		tmp = convert(v1);
		XMVECTOR xv1 = XMLoadFloat3(&tmp);
		tmp = convert(v2);
		XMVECTOR xv2 = XMLoadFloat3(&tmp);

		return DirectX::TriangleTests::Intersects(origin, dir, xv0, xv1, xv2, t);
	}

	std::wstring AnsiToWString(const std::string& str)
	{
		WCHAR buffer[512];
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
		return std::wstring(buffer);
	}

	elm::render::device::DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
		ErrorCode(hr),
		FunctionName(functionName),
		Filename(filename),
		LineNumber(lineNumber)
	{}

	std::wstring DxException::ToString() const
	{
		// Get the string description of the error code.
		_com_error err(ErrorCode);
		//std::wstring msg = err.ErrorMessage();
		return L"";
		//return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
	}
}

