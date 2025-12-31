#include "render/device/dx12/shader.h"

#include "core/debug.h"
#include "core/file_system.h"

#include "render/utils.h"

//#include <libloaderapi.h>
//TODO move to string utils 
#include <string>
#include <format>

namespace render::device {

	//namespace {
	//
	//	constexpr const char* gCompilerPath = "dxcompiler.dll";
	//
	//	static Microsoft::WRL::ComPtr<IDxcUtils> gUtils;
	//	static Microsoft::WRL::ComPtr<IDxcCompiler3> gCompiler3;
	//	static Microsoft::WRL::ComPtr<IDxcValidator> gValidator;
	//	static Microsoft::WRL::ComPtr<IDxcIncludeHandler> gDefaultIncludeHandler;
	//
	//	bool openFile(const char* path, Microsoft::WRL::ComPtr<IDxcBlobEncoding>& out)
	//	{
	//		core::File file;
	//		if (!file.open(path, core::File::Read))
	//		{
	//			return false;
	//		}
	//
	//		string buffer;
	//		buffer.resize(file.length());
	//		file.read(buffer.data(), file.length());
	//
	//		HRESULT hr = gUtils->CreateBlob(buffer.data(), (int)buffer.size(), 0, out.GetAddressOf());
	//		if (!SUCCEEDED(hr))
	//		{
	//			return false;
	//		}
	//		return true;
	//	}
	//
	//	class CustomIncludeHandler : public IDxcIncludeHandler
	//	{
	//	public:
	//		HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
	//		{
	//			Microsoft::WRL::ComPtr<IDxcBlobEncoding> pSource;
	//			if (openFile(core::UnicodeToMultibyte(pFilename).result(), pSource))
	//			{
	//				*ppIncludeSource = pSource.Detach();
	//				return S_OK;
	//			}
	//			return E_FAIL;
	//		}
	//
	//		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override { return E_NOINTERFACE; }
	//		ULONG STDMETHODCALLTYPE AddRef(void) override { return 0; }
	//		ULONG STDMETHODCALLTYPE Release(void) override { return 0; }
	//	};
	//
	//	class CompileArguments
	//	{
	//	public:
	//		void addArgument(const char* argument, const char* pValue = nullptr)
	//		{
	//			
	//			arguments_.push_back(core::MultibyteToUnicode(argument).result());
	//			if (pValue)
	//				arguments_.push_back(core::MultibyteToUnicode(pValue).result());
	//		}	
	//
	//		void addArgument(const wchar_t* argument, const wchar_t* value = nullptr)
	//		{
	//			arguments_.push_back(argument);
	//			if (value)
	//				arguments_.push_back(value);
	//		}
	//		
	//		void addDefine(const char* define, const char* value = nullptr)
	//		{
	//			if (strstr(define, "=") != nullptr)
	//				addArgument("-D", define);
	//			else
	//				addArgument("-D", std::format("%s=%s", define, value ? value : "1").c_str());
	//		}
	//		
	//		const wchar_t** arguments()
	//		{
	//			argumentArr_.reserve(count());
	//			for (const auto& arg : arguments_)
	//				argumentArr_.push_back(arg.c_str());
	//			return argumentArr_.data();
	//		}
	//		
	//		size_t count() const
	//		{
	//			return arguments_.size();
	//		}
	//
	//		string toString() const
	//		{
	//			string str;
	//			for (const std::wstring& arg : arguments_)
	//				str += std::format(" %s", core::UnicodeToMultibyte(arg.c_str()).result());
	//			return str;
	//		}
	//
	//	private:
	//		vector<const wchar_t*> argumentArr_;
	//		vector<std::wstring> arguments_;
	//	};
	//
	//	static void LoadDXC()
	//	{
	//		using DxcCreateInstanceFn = decltype(&::DxcCreateInstance);
	//		
	//		HMODULE lib = LoadLibraryA(gCompilerPath);
	//		DxcCreateInstanceFn CreateInstance = (DxcCreateInstanceFn)GetProcAddress(lib, "DxcCreateInstance");
	//
	//		ThrowIfFailed(CreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(gUtils.GetAddressOf())));
	//		ThrowIfFailed(CreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(gCompiler3.GetAddressOf())));
	//		ThrowIfFailed(CreateInstance(CLSID_DxcValidator, IID_PPV_ARGS(gValidator.GetAddressOf())));
	//		ThrowIfFailed(gUtils->CreateDefaultIncludeHandler(gDefaultIncludeHandler.GetAddressOf()));
	//		ELM_LOG(Info, "Loaded %s", gCompilerPath);
	//	}
	//
	//	ShaderManager g_shaderManager;
	//}
	//
	ShaderManager::ShaderManager()
	{
	//	LoadDXC();
	}
	//
	//Shader* ShaderManager::shader(const char* path, const char* enterPoint, const char* target)
	//{
	//	Microsoft::WRL::ComPtr<IDxcBlobEncoding> pSource;
	//
	//	if (!openFile(path, pSource))
	//	{
	//		return nullptr;
	//	}
	//	
	//	DxcBuffer sourceBuffer;
	//	sourceBuffer.Ptr = pSource->GetBufferPointer();
	//	sourceBuffer.Size = pSource->GetBufferSize();
	//	sourceBuffer.Encoding = DXC_CP_ACP;
	//	
	//	CompileArguments arguments;
	//	arguments.addArgument(path); 
	//	arguments.addArgument("-E");
	//	arguments.addArgument(enterPoint);
	//	arguments.addArgument("-I");
	//	arguments.addArgument("res/shaders/");
	//	arguments.addArgument("-T");
	//	arguments.addArgument(target);
	//
	//	CustomIncludeHandler includeHandler;
	//	Microsoft::WRL::ComPtr<IDxcResult> compileResult;
	//	ThrowIfFailed(gCompiler3->Compile(&sourceBuffer, arguments.arguments(), arguments.count(), &includeHandler, IID_PPV_ARGS(compileResult.GetAddressOf())));
	//
	//	string ErrorMessage;
	//	Microsoft::WRL::ComPtr<IDxcBlobUtf8> pErrors;
	//	if (SUCCEEDED(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr)))
	//	{
	//		if (pErrors && pErrors->GetStringLength())
	//		{
	//			ErrorMessage = (char*)pErrors->GetStringPointer();
	//			ELM_LOG(Error, "Can`t compile [%s] [%s]\n", arguments.toString().c_str(), ErrorMessage.c_str());
	//		}
	//	}
	//
	//	auto shader = makeUnique<Shader>();
	//	ThrowIfFailed(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shader->blob.GetAddressOf()), nullptr));
	//	auto r = shaders_.emplace(string(path) + string(enterPoint), std::move(shader));
	//	ASSERT(r.second);
	//
	//	return r.first->second.get();
	//}
	//
	//Shader* ShaderManager::getShader(const char* path, const char* enterPoint, const char* target)
	//{
	//	return g_shaderManager.shader(path, enterPoint, target);
	//}
	Shader* ShaderManager::shader(const char* path, const char* enterPoint, const char* target)
	{
		return nullptr;
	}
	Shader* ShaderManager::getShader(const char* path, const char* enterPoint, const char* target)
	{
		return nullptr;
	}
}