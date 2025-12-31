#include "render/image.h"

#include "core/debug.h"
#include "core/math_utils.h"

#include "render/utils.h"

namespace render {

	namespace {
		
		uint64 textureMipByteSize(ResourceFormat format, uint32 width, uint32 height, uint32 depth, uint32 mipIndex)
		{
			return slicePitch(format, width, height, mipIndex) * core::Max(1u, depth >> mipIndex);
		}

		uint64 textureByteSize(ResourceFormat format, uint32 width, uint32 height, uint32 depth, uint32 numMips)
		{
			uint64 size = 0;
			for (uint32 mipLevel = 0; mipLevel < numMips; ++mipLevel)
			{
				size += textureMipByteSize(format, width, height, depth, mipLevel);
			}
			return size;
		}
	}

	bool Image::load(const char* path)
	{
		core::File file;
		if (file.open(path, core::File::Read))
		{
			return loadDDS(file);
		}

		return false;
	}

	bool Image::loadDDS(core::File& file)
	{
		// .DDS subheader.
#pragma pack(push,1)
		struct PixelFormatHeader
		{
			uint32 dwSize;
			uint32 dwFlags;
			uint32 dwFourCC;
			uint32 dwRGBBitCount;
			uint32 dwRBitMask;
			uint32 dwGBitMask;
			uint32 dwBBitMask;
			uint32 dwABitMask;
		};
#pragma pack(pop)

		// .DDS header.
#pragma pack(push,1)
		struct FileHeader
		{
			uint32 dwSize;
			uint32 dwFlags;
			uint32 dwHeight;
			uint32 dwWidth;
			uint32 dwLinearSize;
			uint32 dwDepth;
			uint32 dwMipMapCount;
			uint32 dwReserved1[11];
			PixelFormatHeader ddpf;
			uint32 dwCaps;
			uint32 dwCaps2;
			uint32 dwCaps3;
			uint32 dwCaps4;
			uint32 dwReserved2;
		};
#pragma pack(pop)

		// .DDS 10 header.
#pragma pack(push,1)
		struct DX10FileHeader
		{
			uint32 dxgiFormat;
			uint32 resourceDimension;
			uint32 miscFlag;
			uint32 arraySize;
			uint32 reserved;
		};
#pragma pack(pop)
		enum DDS_CAP_ATTRIBUTE
		{
			DDSCAPS_COMPLEX = 0x00000008U,
			DDSCAPS_TEXTURE = 0x00001000U,
			DDSCAPS_MIPMAP = 0x00400000U,
			DDSCAPS2_VOLUME = 0x00200000U,
			DDSCAPS2_CUBEMAP = 0x00000200U,
		};

		auto MakeFourCC = [](uint32 a, uint32 b, uint32 c, uint32 d) { return a | (b << 8u) | (c << 16u) | (d << 24u); };

		constexpr const char pMagic[] = "DDS ";

		char magic[4];
		file.read(magic, 4);
		if (memcmp(pMagic, magic, 4) != 0)
		{
			return false;
		}

		FileHeader header;
		file.read(&header, sizeof(FileHeader));
		if (header.dwSize == sizeof(FileHeader) && header.ddpf.dwSize == sizeof(PixelFormatHeader))
		{
			uint32 bpp = header.ddpf.dwRGBBitCount;

			uint32 fourCC = header.ddpf.dwFourCC;
			bool hasDxgi = fourCC == MakeFourCC('D', 'X', '1', '0');

			DX10FileHeader dx10Header{};
			if (hasDxgi)
			{
				file.read(&dx10Header, sizeof(DX10FileHeader));

				format_ = convertFormat(static_cast<DXGI_FORMAT>(dx10Header.dxgiFormat));
			}
			else
			{
				switch (fourCC)
				{
				case MakeFourCC('B', 'C', '4', 'U'):	format_ = ResourceFormat::BC4_UNORM;		break;
				case MakeFourCC('D', 'X', 'T', '1'):	format_ = ResourceFormat::BC1_UNORM;		break;
				case MakeFourCC('D', 'X', 'T', '3'):	format_ = ResourceFormat::BC2_UNORM;		break;
				case MakeFourCC('D', 'X', 'T', '5'):	format_ = ResourceFormat::BC3_UNORM;		break;
				case MakeFourCC('B', 'C', '5', 'U'):	format_ = ResourceFormat::BC5_UNORM;		break;
				case MakeFourCC('A', 'T', 'I', '2'):	format_ = ResourceFormat::BC5_UNORM;		break;
				case 0:
					if (bpp == 32)
					{
						auto rgbMask = [=](uint32 r, uint32 g, uint32 b, uint32 a)
							{
								return header.ddpf.dwRBitMask == r && header.ddpf.dwGBitMask == g &&
									header.ddpf.dwBBitMask == b && header.ddpf.dwABitMask == a;
							};

						if (rgbMask(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
						{
							format_ = ResourceFormat::RGBA8_UNORM;
						}
						else if (rgbMask(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
						{
							format_ = ResourceFormat::BGRA8_UNORM;
						}
						else
						{
							return false;
						}
					}
					break;
				default:
					return false;
				}
			}

			bool isCubemap = (header.dwCaps2 & 0x0000FC00U) != 0 || (hasDxgi && (dx10Header.miscFlag & 0x4) != 0);
			uint32 imageChainCount = 1;
			if (isCubemap)
			{
				imageChainCount = 6;
				isCubemap_ = true;
			}
			else if (hasDxgi && dx10Header.arraySize > 1)
			{
				imageChainCount = dx10Header.arraySize;
				isArray_ = true;
			}
			ASSERT(isArray_ == false);

			width_		= core::Max(1u, header.dwWidth);
			height_		= core::Max(1u, header.dwHeight);
			depth_		= core::Max(1u, header.dwDepth);
			mipLevels_	= core::Max(1u, header.dwMipMapCount);
			Image* current = this;
			for (uint32 i = 0; i < imageChainCount; ++i)
			{
				current->data_.resize(textureByteSize(format_, width_, height_, depth_, mipLevels_));
				file.read(current->data_.data(), static_cast<uint32>(current->data_.size()));
				if (i < imageChainCount - 1)
				{
					current->next_ = makeUnique<Image>();
					current->next_->format_ = format_;
					current = current->next_.get();
				}
			}

			return true;
		}
		return false;
	}

	const unsigned char* Image::data(uint32 mip) const
	{
		uint64 offset = 0;
		for (uint32 m = 0; m < mip; ++m)
		{
			offset += textureMipByteSize(format_, width_, height_, depth_, m);
		}
		
		return (data_.data()) + offset;
	}
}