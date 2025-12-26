#include "render/common.h"


#include <d3d12.h>
#include <dxgidebug.h>
#include <wrl.h>

namespace render {

	void SetObjectName(ID3D12Object* pObject, const char* pName)
	{
		if (pObject)
			pObject->SetPrivateData(WKPDID_D3DDebugObjectName, (size_t)strlen(pName) + 1, pName);
	}

	std::wstring AnsiToWString(const string& str)
	{
		WCHAR buffer[512];
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
		return std::wstring(buffer);
	}
}