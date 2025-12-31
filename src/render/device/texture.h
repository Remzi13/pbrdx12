#pragma once

#include "core/colors.h"

#include "render/utils.h"

namespace render {

	using namespace core;

	enum class TextureType : uint8
	{		
		Texture2D,
		TextureCube,
	};

	enum class TextureFlag : uint8
	{
		None = 0,
		UnorderedAccess = 1 << 0,
		ShaderResource = 1 << 1,
		RenderTarget = 1 << 2,
		DepthStencil = 1 << 3,
		sRGB = 1 << 4,
	};

	inline constexpr TextureFlag operator| (TextureFlag  Lhs, TextureFlag Rhs)
	{
		return (TextureFlag)((__underlying_type(TextureFlag))Lhs | (__underlying_type(TextureFlag))Rhs);
	}

	struct TextureDesc
	{
		uint32				Width{ 1 };
		uint32				Height{ 1 };
		uint32				Mips{ 1 };
		uint32				SampleCount{ 1 };
		ResourceFormat		Format{ ResourceFormat::Unknown };
		TextureType			Type{ TextureType::Texture2D };
		Color				ClearColor{ Colors::Black };
		TextureFlag			Flags{ TextureFlag::None };

		static TextureDesc create2D(uint32 width, uint32 height, ResourceFormat format, Color clearColor, TextureFlag flags, uint32 sampleCount = 1);
	};

	struct TextureSRVDesc
	{
		TextureSRVDesc(uint8 mipLevel, uint8 numMipLevels)
			: MipLevel(mipLevel), NumMipLevels(numMipLevels)
		{}

		uint8 MipLevel;
		uint8 NumMipLevels;

		bool operator==(const TextureSRVDesc& other) const
		{
			return MipLevel == other.MipLevel &&
				NumMipLevels == other.NumMipLevels;
		}
	};

	class Texture 
	{
	public:
		explicit Texture(const TextureDesc& desc)
			: desc_(desc) {}

		virtual ~Texture() = default;

		uint32 width() const { return desc_.Width; }
		uint32 height() const { return desc_.Height; }

		const Color& clearColor() const { return desc_.ClearColor; }
		const TextureDesc& desc() const { return desc_; }

		virtual uint32 srvIndex() const = 0;

	private:
		TextureDesc desc_;
	};
}