#pragma once

#include "core/std_types.h"
#include "core/memory.h"

#include <wrl.h>

#include "dxc/dxcapi.h"
#include "d3dx12/d3dx12.h"
#include "d3dx12/d3dx12_root_signature.h"
#include "d3dx12/d3dx12_core.h"

namespace render::device {
	
	struct Shader
	{
		//Microsoft::WRL::ComPtr<ID3DBlob> blob;
		Microsoft::WRL::ComPtr<IDxcBlob> blob;
	};

	class ShaderManager
	{
	public:
		ShaderManager();

		Shader* shader(const char* path, const char* enterPoint, const char* target);

		static Shader* getShader(const char* path, const char* enterPoint, const char* target);
	private:
		map<string, UniquePtr<Shader> > shaders_;
	};
}
