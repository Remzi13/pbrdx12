#pragma once

#include "d3dx12/d3dx12.h"

using Microsoft::WRL::ComPtr;

template<typename T>
class ConstantBuffer
{
public:
	void create(ID3D12Device* device, const std::wstring& name)
	{
		D3D12_HEAP_PROPERTIES uploadHeapProps =
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC cbDesc =
			CD3DX12_RESOURCE_DESC::Buffer(alignedSize());

		if (FAILED(device->CreateCommittedResource(
			&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, // Буфер загрузки всегда в этом
			// состоянии
			nullptr, IID_PPV_ARGS(&resource_)))) {
			throw std::runtime_error("Failed to create Constant Buffer.");
		}

		// Маппим ресурс, чтобы получить постоянный указатель на память GPU
		// Для буфера загрузки (UPLOAD) можно маппить один раз и не анмапить
		if (FAILED(resource_->Map(
			0, nullptr, reinterpret_cast<void**>(&begin_)))) {
			throw std::runtime_error("Failed to map Constant Buffer.");
		}

		resource_->SetName(name.c_str());
	}

	void release()
	{
		if (resource_ && begin_)
		{
			// Первый и второй аргументы (диапазоны) можно оставить nullptr
			resource_->Unmap(0, nullptr);
			resource_ = nullptr;
			begin_ = nullptr;
		}
	}

	~ConstantBuffer()
	{
		release();
	}

	void update(const T& newData)
	{
		data_ = newData;
		// Копируем обновленные данные на GPU по замапленному указателю
		memcpy(begin_, &data_, sizeof(T));
	}

	D3D12_GPU_VIRTUAL_ADDRESS address() const
	{
		return resource_->GetGPUVirtualAddress();
	}

	const T& data() const
	{
		return data_;
	}

private:
	static size_t alignedSize() {
		// Проверяем, что размер T кратен 256. Если нет, округляем вверх.
		const size_t alignment = 256;
		size_t size = sizeof(T);
		return (size + alignment - 1) & ~(alignment - 1);
	}

private:
	T data_;
	ComPtr<ID3D12Resource> resource_;
	BYTE* begin_ = nullptr;
};


template<typename T>
class StructuredBuffer
{
public:
	void create(ID3D12Device* device, const T* data, std::uint32_t count, const std::wstring& name)
	{
		// 1. Сохраняем метаданные
		count_ = count;
		bufferSize_ = sizeof(T) * count_;

		// 2. Создаем ресурс на GPU (размер равен общему размеру массива)
		D3D12_HEAP_PROPERTIES uploadHeapProps =
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
				
		D3D12_RESOURCE_DESC sbDesc =
			CD3DX12_RESOURCE_DESC::Buffer(bufferSize_);

		if (FAILED(device->CreateCommittedResource(
			&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &sbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&resource_)))) {
			throw std::runtime_error("Failed to create Structured Buffer.");
		}

		// 3. Маппим ресурс
		if (FAILED(resource_->Map(
			0, nullptr, reinterpret_cast<void**>(&begin_)))) {
			throw std::runtime_error("Failed to map Structured Buffer.");
		}

		resource_->SetName(name.c_str());

		// 4. Копируем начальные данные
		if (data && count > 0) {
			memcpy(begin_, data, bufferSize_);
		}
	}

	// 5. Обновление (для динамической сцены)
	void update(const T* newData, std::uint32_t count)
	{
		if (count != count_ || sizeof(T) * count > bufferSize_) {
			throw std::runtime_error("Structured Buffer size mismatch on update. Re-creation required.");
		}

		if (newData && count > 0) {
			memcpy(begin_, newData, sizeof(T) * count);
		}
	}

	// Этот метод не нужен для SB, он используется для CBV
	/* D3D12_GPU_VIRTUAL_ADDRESS address() const {
		return resource_->GetGPUVirtualAddress();
	} */

	ID3D12Resource* getResource() const {
		return resource_.Get();
	}

	std::uint32_t getCount() const {
		return count_;
	}


	void release()
	{
		if (resource_ && begin_)
		{
			// Первый и второй аргументы (диапазоны) можно оставить nullptr
			resource_->Unmap(0, nullptr);
			resource_ = nullptr;
			begin_ = nullptr;
		}
	}

	~StructuredBuffer()
	{
		release();
	}

private:
	std::uint32_t count_ = 0;
	size_t bufferSize_ = 0;
	ComPtr<ID3D12Resource> resource_;
	BYTE* begin_ = nullptr;
};
