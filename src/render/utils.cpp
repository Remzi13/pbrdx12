#include "render/utils.h"

#include "core/debug.h"
#include "core/math.h"

namespace render {

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
			ASSERT(false);
		}
		return	DXGI_FORMAT_UNKNOWN;
	}

	ResourceState convertState(D3D12_RESOURCE_STATES state)
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
			ASSERT(false);
		}
		return ResourceState::Unknown;
	}

	const FormatInfo& formatInfo(ResourceFormat format)
	{
		const FormatInfo& info = g_formatInfo[(uint32)format];
		ASSERT(info.format == format);
		return info;
	}

	uint64 rowPitch(ResourceFormat format, uint32 width, uint32 mipIndex)
	{
		const FormatInfo& info = formatInfo(format);
		if (info.BlockSize > 0)
		{
			uint64 numBlocks = math::Max(1u, math::divideAndRoundUp(width >> mipIndex, info.BlockSize));
			return numBlocks * info.BytesPerBlock;
		}
		return 0;
	}

	uint64 slicePitch(ResourceFormat format, uint32 width, uint32 height, uint32 mipIndex)
	{
		const FormatInfo& info = formatInfo(format);
		if (info.BlockSize > 0)
		{
			uint64 numBlocksX = math::Max(1u, math::divideAndRoundUp(width >> mipIndex, info.BlockSize));
			uint64 numBlocksY = math::Max(1u, math::divideAndRoundUp(height >> mipIndex, info.BlockSize));
			return numBlocksX * numBlocksY * info.BytesPerBlock;
		}
		return 0;
	}

}