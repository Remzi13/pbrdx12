#pragma once 

#include "core/std_types.h"
#include "core/memory.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h> 
#include <winerror.h>
#include <wrl/client.h> 

#include "dxc/dxcapi.h"
#include "d3dx12/d3dx12.h"
#include "d3dx12/d3dx12_root_signature.h"
#include "d3dx12/d3dx12_core.h"

#include <rpc.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include <d3d12.h>

#include <stdexcept>

namespace render {
	std::wstring AnsiToWString(const string& str);
}

class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
		ErrorCode(hr),
		FunctionName(functionName),
		Filename(filename),
		LineNumber(lineNumber)
	{
	}
	std::wstring ToString()const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};


#ifndef ThrowIfFailed
#define ThrowIfFailed(x)																	\
{																							\
	HRESULT hr__ = (x);																		\
	std::wstring wfn = render::AnsiToWString(__FILE__);										\
	if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); }						\
}
#endif

namespace render {


	using ID3D12ResourceX = ID3D12Resource;

	static constexpr int MAX_ROOT_SIGNATURE_PARAM = 8;

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

	
	class Device;

	void SetObjectName(ID3D12Object* pObject, const char* pName);

	class DeviceObject
	{
	public:
		DeviceObject(Device* parent)
			: parent_(parent)
		{
		}
		virtual ~DeviceObject() = default;

		Device* parent() const { return parent_; }

	private:
		Device* parent_;
	};

}