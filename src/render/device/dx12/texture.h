#pragma once

#include "render/device/texture.h"

#include "render/device/dx12/dx12.h"
#include "render/device/dx12/resource.h"
#include "render/image.h"

namespace elm::render {
	
	class TextureDx12 : public Texture, public DeviceResource
	{
	public:
		TextureDx12(GraphicsDevice* device, TextureDesc desc, ID3D12ResourceX* resource);

		void setSRV(const SharedPtr<ShaderResourceView>& srv) { srv_ = srv; }
		ShaderResourceView* srv() const { return srv_.get(); }
		uint32 srvIndex() const override;

	private:
		SharedPtr<ShaderResourceView> srv_;
	};

	SharedPtr<Texture> createTexture(GraphicsDevice* device, const Image& imgae, const char* name);
}