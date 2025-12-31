#include "render/device/dx12/texture.h"

#include "render/device/dx12/device.h"

namespace render {	

	TextureDesc TextureDesc::create2D(uint32 width, uint32 height, ResourceFormat format, core::Color clearColor, TextureFlag flags, uint32 sampleCount )
	{
		TextureDesc desc{};
		desc.Width = width;
		desc.Height = height;
		desc.SampleCount = sampleCount;
		desc.Format = format;
		desc.Type = TextureType::Texture2D;
		desc.ClearColor = clearColor;
		desc.Flags = flags;

		return desc;
	}

	TextureDx12::TextureDx12(GraphicsDevice* device, TextureDesc desc, ID3D12ResourceX* resource) 
		: DeviceResource(device, resource), Texture(desc)
	{}

	uint32 TextureDx12::srvIndex() const
	{
		return srv_ ? srv_->heapIndex() : DescriptorHandle::InvalidHeapIndex;
	}


	SharedPtr<Texture> createTexture(GraphicsDevice* device, const Image& image, const char* name)
	{
		TextureDesc desc;
		desc.Width  = image.width();
		desc.Height = image.height();
		desc.Format = image.format();
		desc.Mips   = image.mipLevel();

		desc.Flags = TextureFlag::ShaderResource;
		desc.Type = image.isCube() ? TextureType::TextureCube : TextureType::Texture2D;
		vector<D3D12_SUBRESOURCE_DATA> subResourceData;
		const Image* img = &image;
		while (img)
		{
			for (uint32 i = 0; i < desc.Mips; ++i)
			{
				D3D12_SUBRESOURCE_DATA& data = subResourceData.emplace_back();
				data.pData = img->data(i);
				data.RowPitch = rowPitch(image.format(), desc.Width, i);
				data.SlicePitch = slicePitch(image.format(), desc.Width, desc.Height, i);
			}
			img = img->next();
		}
		return static_cast<DeviceDx12*>(device)->createTexture(desc, name ? name: "", nullptr, subResourceData);
	}
}