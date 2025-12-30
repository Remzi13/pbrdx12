#include "render/texture.h"

#include "render/descriptor_heap.h"

namespace render {

	TextureDesc TextureDesc::create2D(uint32 width, uint32 height, ResourceFormat format, Color clearColor, TextureFlag flags, uint32 sampleCount)
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

	Texture::Texture(Device* device, TextureDesc desc, ID3D12ResourceX* resource)
		: DeviceResource(device, resource), desc_(desc)
	{
	}

	uint32 Texture::srvIndex() const
	{
		return srv_ ? srv_->heapIndex() : DescriptorHandle::InvalidHeapIndex;
	}


}