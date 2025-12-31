#pragma once

namespace render {
	enum class ShaderType
	{
		Vertex,
		Pixel,
		COUNT,
	};

	enum class ResourceFormat
	{
		Unknown,
		R16_UINT,
		R32_UINT,
		RG32_FLOAT,
		RGB32_FLOAT,
		RGBA8_UNORM,
		RGBA32_FLOAT,

		D24S8,

		BGRA8_UNORM,
		
		BC1_UNORM,
		BC2_UNORM,
		BC3_UNORM,
		BC4_UNORM,
		BC5_UNORM,

		Count
	};

	enum ResourceState
	{
		Unknown,
		Common,
		Present,
		RenderTarget,
		Depth,
	};

	enum PrimitiveTopology
	{
		TriangleList
	};

	enum class BufferFlag : uint8
	{
		None = 0,
		UnorderedAccess = 1 << 0,
		ShaderResource = 1 << 1,
		Upload = 1 << 2,
		Readback = 1 << 3,
		ByteAddress = 1 << 4,
		AccelerationStructure = 1 << 5,
		IndirectArguments = 1 << 6,
		NoBindless = 1 << 7,
	};

	inline constexpr BufferFlag operator| (BufferFlag  Lhs, BufferFlag Rhs) 
	{ 
		// Cast to underlying type to avoid enum class promotion
		return (BufferFlag)((__underlying_type(BufferFlag))Lhs | (__underlying_type(BufferFlag))Rhs); 
	}

}