#pragma once

#include "core/memory.h"

#include "render/common.h"
#include "render/resources.h"

namespace render {

	using namespace memory;

	class Color
	{
	public:
		float r{ 0 };
		float g{ 0 };
		float b{ 0 };
		float a{ 0 };

		Color() {}
		constexpr Color(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) {}
	};

	namespace Colors
	{
		constexpr Color Transparent = Color(0.0f, 0.0f, 0.0f, 0.0f);
		constexpr Color White = Color(1.0f, 1.0f, 1.0f, 1.0f);
		constexpr Color Black = Color(0.0f, 0.0f, 0.0f, 1.0f);
		constexpr Color Red = Color(1.0f, 0.0f, 0.0f, 1.0f);
		constexpr Color Green = Color(0.0f, 1.0f, 0.0f, 1.0f);
		constexpr Color Blue = Color(0.0f, 0.0f, 1.0f, 1.0f);
		constexpr Color Yellow = Color(1.0f, 1.0f, 0.0f, 1.0f);
		constexpr Color Magenta = Color(1.0f, 0.0f, 1.0f, 1.0f);
		constexpr Color Cyan = Color(0.0f, 1.0f, 1.0f, 1.0f);
		constexpr Color Gray = Color(0.5f, 0.5f, 0.5f, 1.0f);
	}

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
		{
		}

		uint8 MipLevel;
		uint8 NumMipLevels;

		bool operator==(const TextureSRVDesc& other) const
		{
			return MipLevel == other.MipLevel &&
				NumMipLevels == other.NumMipLevels;
		}
	};


	class Texture : public DeviceResource
	{
	public:
		Texture(Device* device, TextureDesc desc, ID3D12ResourceX* resource);

		void setSRV(const SharedPtr<ShaderResourceView>& srv) { srv_ = srv; }
		ShaderResourceView* srv() const { return srv_.get(); }
		uint32 srvIndex() const;

		const TextureDesc& desc() const { return desc_; }

	private:
		SharedPtr<ShaderResourceView> srv_;
		TextureDesc desc_;
	};

	//SharedPtr<Texture> createTexture(Device* device, const Image& imgae, const char* name);
}