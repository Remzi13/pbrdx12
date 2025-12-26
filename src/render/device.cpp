#include "render/device.h"

#include "render/common.h"


using Microsoft::WRL::ComPtr;

namespace render {

	bool Device::init()
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
		}

		ComPtr<IDXGIFactory4> factory;
		if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,
			IID_PPV_ARGS(&factory)))) {
			throw std::runtime_error("Failed to create DXGI Factory.");
		}

		ComPtr<IDXGIAdapter1> hardwareAdapter;
		for (UINT adapterIndex = 0;
			factory->EnumAdapters1(adapterIndex, &hardwareAdapter) !=
			DXGI_ERROR_NOT_FOUND;
			++adapterIndex) {
			DXGI_ADAPTER_DESC1 desc;
			hardwareAdapter->GetDesc1(&desc);

			// Пропускаем программный адаптер Microsoft (это эмулятор)
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				continue;
			}


			if (FAILED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0,
				IID_PPV_ARGS(&device_)))) {
				throw std::runtime_error("Failed to create D3D12 Device.");
			}
		}

		if (FAILED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&device_)))) {
			throw std::runtime_error("Failed to create D3D12 Device.");
		}

		// --- 4. Создание "Очереди Команд" (Command Queue) ---	
		graphicsQueue_ = makeUnique<CommandQueue>(this, CommandQueue::Type::GRAPHICS);

		return false;
	}

	ID3D12Device* Device::device() const
	{
		return device_.Get();
	}
}
