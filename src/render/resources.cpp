#include "render/resources.h"

#include "render/device.h"

namespace render {

	DeviceResource::DeviceResource(Device* device, ID3D12ResourceX* resource) : DeviceObject(device), resource_(resource)
	{
	}

	DeviceResource::~DeviceResource()
	{
		if (resource_)
		{
			if (immediateDelete_)
			{
				resource_->Release();
			}
			else
			{
				parent()->deferReleaseObject(resource_);
			}
			resource_ = nullptr;
			//m_count.fetch_add( -1 );
		}
	}

	void DeviceResource::setName(const char* name)
	{
		SetObjectName(resource_, name);
		name_ = name;
	}
}