#pragma once

#include "core/std_types.h"
#include "core/memory.h"

#include <wrl.h>

#include "d3dcommon.h"

namespace elm::render::device {
	using namespace memory;

	struct Shader
	{
		Microsoft::WRL::ComPtr<ID3DBlob> blob;
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
