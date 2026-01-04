#pragma once

#include "core/std_types.h"

#include "d3dx12/d3dx12.h"

namespace render {
	using ID3D12ResourceX = ID3D12Resource;
	using ID3D12DeviceX = ID3D12Device5;
	
	inline void SetObjectName(ID3D12Object* pObject, const char* pName)
	{
		if (pObject)
			pObject->SetPrivateData(WKPDID_D3DDebugObjectName, (uint32)strlen(pName) + 1, pName);
	}
	
	namespace utils {
		bool isTransitionAllowed(D3D12_COMMAND_LIST_TYPE commandlistType, D3D12_RESOURCE_STATES state);
		string resourceStateToString(D3D12_RESOURCE_STATES state);
		string commandlistTypeToString(D3D12_COMMAND_LIST_TYPE type);
	}

	
}
