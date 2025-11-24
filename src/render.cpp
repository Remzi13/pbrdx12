//
// Created by zakkh on 10.11.2025.
//
#include "render.h"

#include "dxc/dxcapi.h"
#include "d3dx12/d3dx12.h"
#include "d3dx12/d3dx12_root_signature.h"
#include "d3dx12/d3dx12_core.h"

#include <wrl/client.h> // Для "умных" указателей ComPtr

#include <rpc.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>

#include <exception>
#include <stdexcept>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>

#include <stdint.h>

#include "vector.h"

#include "buffers.h"

using Microsoft::WRL::ComPtr;

ComPtr<IDxcUtils> g_dxcUtils;
ComPtr<IDxcCompiler3> g_dxcCompiler;

// Функция компиляции HLSL в DXIL байт-код
ComPtr<IDxcBlob> CompileShader(
	const std::wstring& filename,
	const std::wstring& entryPoint,
	const std::wstring& targetProfile)
{
	// 1. Загрузка исходного кода из файла
	ComPtr<IDxcBlobEncoding> source;
	// Используем g_dxcUtils для загрузки файла
	if (FAILED(g_dxcUtils->LoadFile(filename.c_str(), nullptr, &source)))
	{
		throw std::runtime_error("Failed to load shader file.");
	}

	// 2. Аргументы компиляции
	std::vector<LPCWSTR> arguments;

	// Target Profile: cs_6_0 (Compute Shader Model 6.0)
	arguments.push_back(L"-T");
	arguments.push_back(targetProfile.c_str());

	// Entry Point: CSMain
	arguments.push_back(L"-E");
	arguments.push_back(entryPoint.c_str());

	arguments.push_back(L"-Zi");   // Включение отладочной информации
	arguments.push_back(L"-Od");   // Отключение оптимизаций (часто помогает с подписью)
	arguments.push_back(L"-Qstrip_reflect"); // Удаление информации о рефлексии
	arguments.push_back(L"-all-resources-bound"); // Для Compute Shader

	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = source->GetBufferPointer();
	sourceBuffer.Size = source->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_ACP; // Кодировка

	// 3. Вызов компилятора
	ComPtr<IDxcResult> result;
	if (FAILED(g_dxcCompiler->Compile(
		&sourceBuffer,              // Исходный код шейдера
		arguments.data(),           // Аргументы
		(UINT)arguments.size(),
		nullptr,                    // Include Handler
		IID_PPV_ARGS(&result)
	)))
	{
		throw std::runtime_error("DxcCompiler::Compile failed.");
	}

	// 4. Проверка ошибок
	HRESULT hr;
	result->GetStatus(&hr);

	ComPtr<IDxcBlobUtf8> errors;
	// Используем GetOutput для получения ошибок (работает с большинством версий)
	result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

	if (FAILED(hr))
	{
		std::string errorMsg = "Shader compilation failed (HR: " + std::to_string(hr) + "):\n";
		if (errors && errors->GetStringPointer())
		{
			errorMsg += errors->GetStringPointer();
		}
		throw std::runtime_error(errorMsg);
	}

	// 5. Получение скомпилированного байт-кода
	ComPtr<IDxcBlob> shaderBlob;

	// Используем GetOutput для получения объекта шейдера (это наиболее надежно)
	if (FAILED(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr)))
	{
		throw std::runtime_error("Failed to get compiled shader object.");
	}

	return shaderBlob;
}

struct SceneParameters
{
	Vector3 camera_center;
	float _pad0;

	Vector3 pixel00_loc;
	float _pad1;

	Vector3 pixel_delta_u;
	float _pad2;

	Vector3 pixel_delta_v;
	float _pad3;

	uint32_t primitiveCount;
	uint32_t trianglesCount;
	uint32_t sampleCount;
	float _pad_end[1];
};

SceneParameters calcSceneParam(std::uint16_t width, std::uint16_t height, const Vector3& cameraCenter)
{
	const float FOCAL_LENGTH = 1.0f;     // Соответствует leftTop.z
	const float VIEWPORT_HEIGHT = 1.0f;  // Изменение: высота от -0.5 до 0.5 (Всего 1.0)

	const float aspectRatio = (float)width / (float)height;
	const float viewport_width = VIEWPORT_HEIGHT * aspectRatio;

	// 2. Векторы U и V
	// viewport_u (шаг по X) и viewport_v (шаг по Y, отрицательный для инверсии)
	// Это соответствует направлению U/V, используемому в цикле
	const Vector3 viewport_u = { viewport_width, 0.0f, 0.0f };
	const Vector3 viewport_v = { 0.0f, -VIEWPORT_HEIGHT, 0.0f }; // V направлен вниз

	// 3. Направление Взгляда
	// Камера в (0, 0, 0) смотрит на Z=1.0
	const Vector3 look_direction = { 0.0f, 0.0f, FOCAL_LENGTH };
	// (Ваш оригинальный код уже использовал FOCAL_LENGTH, это правильно)

	// 4. Верхний Левый Угол (Viewport Upper Left)
	// cameraCenter + look_direction = (0, 0, 1)
	// - viewport_u / 2.0f = Сдвиг влево на половину ширины
	// - viewport_v / 2.0f = Сдвиг вверх на 0.5
	const Vector3 viewport_upper_left = cameraCenter + look_direction - viewport_u / 2.0f - viewport_v / 2.0f;

	// 5. Центр Пикселя (pixel00_loc)
	// Это центр первого пикселя.
	const Vector3 pixel_delta_u = viewport_u / (float)width;
	const Vector3 pixel_delta_v = viewport_v / (float)height;
	const Vector3 pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

	SceneParameters params;
	params.camera_center = cameraCenter;
	params.pixel00_loc = pixel00_loc;
	params.pixel_delta_v = pixel_delta_v;
	params.pixel_delta_u = pixel_delta_u;

	return params;
}

ConstantBuffer<SceneParameters> g_buffer;

bool Render::init(HWND hwnd, const Scene& scene)
{ 
	width_ = scene.width();
	height_ = scene.height();

	// --- 1. Включение слоя отладки ---
	// Это ОБЯЗАТЕЛЬНО для разработки. Он будет сообщать об ошибках в DX12.
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
	}

	// --- 2. Создание "Фабрики" DXGI ---
	// Фабрика нужна для "производства" других объектов DXGI (адаптеры, SwapChain)
	ComPtr<IDXGIFactory4> factory;
	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,
		IID_PPV_ARGS(&factory)))) {
		throw std::runtime_error("Failed to create DXGI Factory.");
	}

	// --- 3. Поиск и создание "Устройства" (Device) ---
	// Ищем подходящую видеокарту (адаптер)
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

		// Пытаемся создать устройство. Если получилось - это наш адаптер.
		if (FAILED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&device_)))) {
			throw std::runtime_error("Failed to create D3D12 Device.");
		}
	}

	// Создаем само устройство
	if (FAILED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(&device_)))) {
		throw std::runtime_error("Failed to create D3D12 Device.");
	}

	// --- 4. Создание "Очереди Команд" (Command Queue) ---
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type =
		D3D12_COMMAND_LIST_TYPE_DIRECT; // Прямое выполнение (для рендеринга)
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	if (FAILED(device_->CreateCommandQueue(&queueDesc,
		IID_PPV_ARGS(&commandQueue_)))) {
		throw std::runtime_error("Failed to create Command Queue.");
	}

	// --- 5. Создание Цепочки Обмена (Swap Chain) ---
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = width_;
	swapChainDesc.Height = height_;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Стандартный формат цвета
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect =
		DXGI_SWAP_EFFECT_FLIP_DISCARD; // Современный метод переключения буферов
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	if (FAILED(factory->CreateSwapChainForHwnd(
		commandQueue_.Get(), // Swap Chain должен знать нашу Command Queue
		hwnd, &swapChainDesc, nullptr, nullptr, &swapChain))) {
		throw std::runtime_error("Failed to create Swap Chain.");
	}

	// Получаем интерфейс SwapChain3, т.к. он позволяет нам узнать индекс текущего
	// буфера
	if (FAILED(swapChain.As(&swapChain_))) {
		throw std::runtime_error("Failed to get SwapChain3 interface.");
	}

	// --- 6. Создание Кучи Дескрипторов для Render Targets (RTV Heap) ---
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors =
		FrameCount; // По одному дескриптору на каждый буфер кадра
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // RTV = Render Target View
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	if (FAILED(device_->CreateDescriptorHeap(&rtvHeapDesc,
		IID_PPV_ARGS(&rtvHeap_)))) {
		throw std::runtime_error("Failed to create RTV Descriptor Heap.");
	}

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 2;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // Тип кучи
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // Видима для шейдера

	// Узнаем размер одного дескриптора в этой куче (варьируется от GPU к GPU)
	rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// --- 7. Создание представлений (Views) для Буферов Кадра ---
	// Мы сообщаем DX12, как "видеть" каждый из двух буферов как цель рендеринга.

	// Начинаем с первого дескриптора в куче
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
		rtvHeap_->GetCPUDescriptorHandleForHeapStart();

	for (UINT n = 0; n < FrameCount; n++) {
		// 1. Получаем сам буфер (ресурс) из Swap Chain
		if (FAILED(swapChain_->GetBuffer(n, IID_PPV_ARGS(&renderTargets_[n])))) {
			throw std::runtime_error("Failed to get buffer from Swap Chain.");
		}

		// 2. Создаем "Представление" (View) этого ресурса в текущем слоте кучи
		device_->CreateRenderTargetView(renderTargets_[n].Get(), nullptr, rtvHandle);

		// Передвигаем дескриптор на следующий слот в куче (для следующего буфера)
		rtvHandle.ptr += rtvDescriptorSize_;
	}

	// Узнаем, какой буфер кадра (0 или 1) мы должны использовать первым
	frameIndex_ = swapChain_->GetCurrentBackBufferIndex();

	// --- 8. Создание Аллокатора и Списка Команд ---
	// Аллокатор хранит команды
	if (FAILED(device_->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_)))) {
		throw std::runtime_error("Failed to create command allocator.");
	}

	// Список Команд (мы будем использовать его для записи инструкций)
	if (FAILED(device_->CreateCommandList(
		0, // nodeMask (всегда 0 для одиночного GPU)
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator_.Get(), // Аллокатор
		nullptr, // Изначальное PSO (Pipeline State Object), пока нет, ставим
		// nullptr
		IID_PPV_ARGS(&commandList_)))) {
		throw std::runtime_error("Failed to create command list.");
	}

	// Закрываем список команд, пока не будем готовы его заполнить в цикле
	// рендеринга
	commandList_->Close();

	// --- 9. Создание Объектов Синхронизации (Fence) ---
	if (FAILED(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)))) {
		throw std::runtime_error("Failed to create fence.");
	}

	// Создаем событие, которое будет использоваться для сигнализации CPU
	fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent_ == nullptr) {
		throw std::runtime_error("Failed to create fence event.");
	}

	// --- 10. Загрузка скомпилированного шейдера ---
	if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&g_dxcUtils)))) {
		throw std::runtime_error("Failed to create DXC Utils.");
	}

	// Создаем DXC Compiler
	if (FAILED(
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&g_dxcCompiler)))) {
		throw std::runtime_error("Failed to create DXC Compiler.");
	}

	ComPtr<IDxcBlob> computeShader =
		CompileShader(L"../shaders/RayTracing.hlsl", L"CSMain", L"cs_6_0");
	ComPtr<ID3DBlob> shaderBlobForPSO;
	// Используем .As() для запроса совместимого интерфейса ID3DBlob
	if (FAILED(computeShader.As(&shaderBlobForPSO))) {
		throw std::runtime_error(
			"Failed to query ID3DBlob interface from IDxcBlob.");
	}

	// --- 11. Создание Root Signature (Определяет, как данные будут передаваться
	// в шейдер) ---
	// 1. Определение диапазонов
	// Диапазон 1: UAV (Output Texture, u0)
	CD3DX12_DESCRIPTOR_RANGE uavRange;
	uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // 1 дескриптор в u0

	// Диапазон 2: SRV (Structured Buffer, t0)
	CD3DX12_DESCRIPTOR_RANGE srvRange;
	srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0); // 1 дескриптор в t0

	// 2. Создаем массив диапазонов для одной таблицы
	// Таблица будет включать UAV, а затем SRV
	CD3DX12_DESCRIPTOR_RANGE ranges[2];
	ranges[0] = uavRange;
	ranges[1] = srvRange;

	// 3. Определяем параметры Root Signature
	CD3DX12_ROOT_PARAMETER rootParameters[2];

	// ПАРАМЕТР 0: Таблица дескрипторов для UAV (u0) И SRV (t0)
	rootParameters[0].InitAsDescriptorTable(
		2,
		ranges,
		D3D12_SHADER_VISIBILITY_ALL
	);

	// ПАРАМЕТР 1: CBV (Константный Буфер) (регистр b0)
	rootParameters[1].InitAsConstantBufferView(
		0, 0, D3D12_SHADER_VISIBILITY_ALL
	);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		&signature, &error);

	if (FAILED(device_->CreateRootSignature(0, signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature_)))) {
		throw std::runtime_error("Failed to create Root Signature.");
	}

	// --- 12. Создание Compute PSO (Pipeline State Object) ---
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	computePsoDesc.pRootSignature = rootSignature_.Get();

	// Указываем скомпилированный шейдер!
	computePsoDesc.CS.pShaderBytecode = shaderBlobForPSO->GetBufferPointer();
	computePsoDesc.CS.BytecodeLength = shaderBlobForPSO->GetBufferSize();

	if (FAILED(device_->CreateComputePipelineState(&computePsoDesc,
		IID_PPV_ARGS(&pso_)))) {
		throw std::runtime_error("Failed to create Compute PSO.");
	}

	// --- 13. Создание Выходного Ресурса (Texture2D) и UAV Heap ---

	// 1. Создание Кучи Дескрипторов для UAV
	D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
	uavHeapDesc.NumDescriptors = 3;
	uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	uavHeapDesc.Flags =
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // Должен быть виден шейдеру!

	if (FAILED(device_->CreateDescriptorHeap(&uavHeapDesc,
		IID_PPV_ARGS(&uavHeap_)))) {
		throw std::runtime_error("Failed to create UAV Descriptor Heap.");
	}

	// 2. Создание Ресурса (Текстуры), в который будет писать Compute Shader
	D3D12_HEAP_PROPERTIES heapProps =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, width_, height_, 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS // Разрешаем UAV
	);

	if (FAILED(device_->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_COMMON, // Начальное состояние (COMMON)
		nullptr, IID_PPV_ARGS(&computeOutput_)))) {
		throw std::runtime_error("Failed to create Compute Output Resource.");
	}

	// 3. Создание UAV (Представление неупорядоченного доступа)
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Создаем UAV в куче, используя CPU дескриптор
	device_->CreateUnorderedAccessView(
		computeOutput_.Get(), nullptr, &uavDesc,
		uavHeap_->GetCPUDescriptorHandleForHeapStart());

	// --- 14. Создание Readback Buffer для чтения результата на CPU ---

	const UINT totalSize = width_ * height_ * 4;

	D3D12_RESOURCE_DESC textureDesc = computeOutput_->GetDesc();

	// Получаем информацию о размере и расположении данных в буфере
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint = {};
	UINT tempNumRows;
	UINT64 tempRowSizeInBytes;

	// ВЫЗОВ: Получаем необходимый размер (TotalBytes) для нашего буфера чтения
	device_->GetCopyableFootprints(
		&textureDesc,
		0, // Subresource Index (у нас только 0)
		1, // NumSubresources
		0, // BaseOffset (должен быть 0)
		&Footprint, &tempNumRows, &tempRowSizeInBytes,
		&TotalBytes
	);

	RowSizeInBytes = Footprint.Footprint.RowPitch;
	// Свойства кучи: READBACK (для чтения с CPU)
	D3D12_HEAP_PROPERTIES readBackHeapProps =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

	// Описание ресурса: Буфер, размером в TotalBytes
	D3D12_RESOURCE_DESC readBackDesc =
		CD3DX12_RESOURCE_DESC::Buffer(TotalBytes);

	if (FAILED(device_->CreateCommittedResource(
		&readBackHeapProps, D3D12_HEAP_FLAG_NONE, &readBackDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
		IID_PPV_ARGS(&readBackBuffer_)))) {
		throw std::runtime_error("Failed to create Readback Buffer.");
	}

	// --- 15. Создание Буфера Констант (CBV) ---

	const Vector3 camera_center = { 0.0f, 0.0f, 0.0f };

	SceneParameters params = calcSceneParam(width_, height_, camera_center);
	params.primitiveCount = scene.count();
	params.trianglesCount = scene.triangles().size();
	params.sampleCount = scene.samples();

	g_buffer.create(device_.Get(), params, L"SceneParameters");

	createSceneSRV(scene);

	return true;
}

void Render::createSceneSRV(const Scene& scene)
{
	std::vector<Primitive> primitives;
	primitives.reserve( scene.count() );

	for ( const auto& sp : scene.spheres() )
	{
		Primitive p;
		p.type = 0;
		p.radius = sp.radius;
		p.position = Vector4( sp.pos.x(), sp.pos.y(), sp.pos.z(), 0.0 );
		p.color = Vector4( sp.color.x(), sp.color.y(), sp.color.z(), 0.0 );

		primitives.emplace_back( p );
	}

	for ( const auto& pl : scene.planes() )
	{
		Primitive p;
		p.type = 1;
		p.radius = pl.dist;
		p.position = Vector4( pl.normal.x(), pl.normal.y(), pl.normal.z(), 0.0 );
		p.color = Vector4( pl.color.x(), pl.color.y(), pl.color.z(), 0.0 );

		primitives.emplace_back( p );
	}

	std::vector<RTrinangle> triangles;
	triangles.reserve(scene.triangles().size());
	for (const auto& tr : scene.triangles())
	{
		RTrinangle triangle;
		triangle.a = Vector3(tr.a.x(), tr.a.y(), tr.a.z());
		triangle.b = Vector3(tr.b.x(), tr.b.y(), tr.b.z());
		triangle.c = Vector3(tr.c.x(), tr.c.y(), tr.c.z());
		triangle.color = tr.color;

		triangles.emplace_back(triangle);
	}
	

	// Создаем новый с актуальными данными
	scenePrimitives_.create( device_.Get(), primitives.data(), (std::uint32_t)primitives.size(), L"ScenePrimtives" );
	sceneTriangles_.create(device_.Get(), triangles.data(), (std::uint32_t)triangles.size(), L"SceneTriangles");

	{
		UINT elementCount = scenePrimitives_.getCount();
		ID3D12Resource* sbResource = scenePrimitives_.getResource();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = elementCount;
		srvDesc.Buffer.StructureByteStride = sizeof(Primitive);
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		UINT descriptorSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(uavHeap_->GetCPUDescriptorHandleForHeapStart(), 1, descriptorSize);
		device_->CreateShaderResourceView(sbResource, &srvDesc, srvHandle);
	}
	{
		UINT elementCount = sceneTriangles_.getCount();
		ID3D12Resource* sbResource = sceneTriangles_.getResource();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = elementCount;
		srvDesc.Buffer.StructureByteStride = sizeof(RTrinangle);
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		UINT descriptorSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(uavHeap_->GetCPUDescriptorHandleForHeapStart(), 2, descriptorSize);
		device_->CreateShaderResourceView(sbResource, &srvDesc, srvHandle);
	}
	
}

void Render::draw()
{
	// 1. Сбрасываем (Reset) аллокатор и список
	commandAllocator_->Reset();
	commandList_->Reset(commandAllocator_.Get(), pso_.Get());

	// --- 1. Установка Compute Pipeline ---
	commandList_->SetPipelineState(pso_.Get());
	commandList_->SetComputeRootSignature(rootSignature_.Get());


	// Устанавливаем UAV кучу
	ID3D12DescriptorHeap* ppHeaps[] = { uavHeap_.Get() };
	commandList_->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Привязываем UAV к Root Signature (регистр u0)
	commandList_->SetComputeRootDescriptorTable(0, uavHeap_->GetGPUDescriptorHandleForHeapStart());
	// Привязываем CBV к Root Signature (второй параметр, индекс 1)
	commandList_->SetComputeRootConstantBufferView(1, g_buffer.address());

	// --- 2. Барьер: Переводим g_computeOutput в UAV/WRITE ---
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	uavBarrier.Transition.pResource = computeOutput_.Get();
	uavBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
	uavBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	uavBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList_->ResourceBarrier(1, &uavBarrier);

	// --- 3. Диспатч: Запуск вычислительного шейдера ---
	UINT groupX = (width_ + 7) / 8;
	UINT groupY = (height_ + 7) / 8;
	commandList_->Dispatch(groupX, groupY, 1);

	// --- 4. Барьер: Переводим g_computeOutput из UAV в COPY_SOURCE ---
	uavBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	uavBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	commandList_->ResourceBarrier(1, &uavBarrier);

	// --- 5. Копирование результата в Back Buffer И Readback Buffer ---
	frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
	ComPtr<ID3D12Resource> backBuffer = renderTargets_[frameIndex_];

	// --- 5a. Барьер: Back Buffer в COPY_DEST ---
	// ЭТОТ БАРЬЕР ДОЛЖЕН БЫТЬ ПЕРЕД ЛЮБЫМ КОПИРОВАНИЕМ В BACK BUFFER!
	D3D12_RESOURCE_BARRIER copyDestBarrier = {};
	copyDestBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	copyDestBarrier.Transition.pResource = backBuffer.Get();
	copyDestBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	copyDestBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	copyDestBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList_->ResourceBarrier(1, &copyDestBarrier);

	// 5b. КОПИРОВАНИЕ В BACK BUFFER (для отображения на экране)
	// Используем CopyResource, так как Back Buffer и g_computeOutput имеют одинаковые размеры
	commandList_->CopyResource(backBuffer.Get(), computeOutput_.Get());

	// 5c. КОПИРОВАНИЕ В READBACK BUFFER (для сохранения в файл)
	D3D12_TEXTURE_COPY_LOCATION Dst = {};
	Dst.pResource = readBackBuffer_.Get();
	Dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint = {};

	// ВАЖНОЕ ИСПРАВЛЕНИЕ: Используем локальные переменные для вывода.
	// Глобальная RowSizeInBytes (3328) НЕ ДОЛЖНА здесь меняться!
	UINT localNumRows;
	UINT64 localRowSizeInBytes;
	UINT64 localTotalBytes;

	// ВЫЗОВ: Получаем Footprint для команды копирования
	device_->GetCopyableFootprints(
		&computeOutput_->GetDesc(), 0, 1, 0,
		&Footprint,
		&localNumRows,
		&localRowSizeInBytes,
		&localTotalBytes // Используем локальные переменные
	);

	Dst.PlacedFootprint = Footprint;

	D3D12_TEXTURE_COPY_LOCATION Src = {};
	Src.pResource = computeOutput_.Get();
	Src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	Src.SubresourceIndex = 0;

	// Копируем!
	commandList_->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

	// --- 6. Финальный Барьер: Back Buffer в PRESENT ---
	// Готовим Back Buffer к показу (PRESENT)
	copyDestBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	copyDestBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList_->ResourceBarrier(1, &copyDestBarrier);

	// 7. Закрываем список команд
	commandList_->Close();

	ID3D12CommandList* ppCommandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists); // 2. Отправляем команды на GPU

	swapChain_->Present(1, 0); // 3. Показываем результат (present)

	wait();
}

void Render::wait()
{
	// Сохраняем значение, которое GPU должен будет записать в Fence
	const UINT64 fence = fenceValue_;

	// Говорим Command Queue записать это значение, когда все предыдущие команды выполнены
	commandQueue_->Signal(fence_.Get(), fence);
	fenceValue_++;

	// Ждем, пока Fence не достигнет или не превысит сохраненное нами значение
	if (fence_->GetCompletedValue() < fence)
	{
		// Устанавливаем событие, которое будет вызвано при достижении значения
		fence_->SetEventOnCompletion(fence, fenceEvent_);
		// Ждем это событие (Блокировка CPU)
		WaitForSingleObject(fenceEvent_, INFINITE);


		static bool saved = true;
		if (!saved)
		{
			saved = true;

			// 1. Получаем указатель на данные Readback Buffer
			void* pData;
			D3D12_RANGE readRange = { 0, (SIZE_T)TotalBytes }; // TotalBytes из предыдущего шага
			readBackBuffer_->Map(0, &readRange, &pData);

			// 2. Сохранение в файл PPM (очень простой текстовый формат)
			std::ofstream outfile("output.ppm", std::ios::out | std::ios::binary);
			if (outfile.is_open())
			{
				// Заголовок PPM
				outfile << "P3\n" << width_ << " " << height_ << "\n255\n";

				const unsigned char* src = (const unsigned char*)pData;
				// Данные
				for (int y = 0; y < height_; ++y)
				{
					// Указатель на начало текущей строки
					const unsigned char* rowStart = src + y * RowSizeInBytes;

					// Проходим по пикселям в этой строке
					for (int x = 0; x < width_; ++x)
					{
						// Пиксели в формате R8G8B8A8 (4 байта на пиксель)
						const unsigned char* pixel = rowStart + x * 4;

						unsigned char r = pixel[0]; // R
						unsigned char g = pixel[1]; // G
						unsigned char b = pixel[2]; // B

						outfile << (int)r << " " << (int)g << " " << (int)b << " ";
					}
					outfile << "\n";
				}
				outfile.close();

				// Сообщаем о сохранении
				printf("Image saved to output.ppm\n");
			}
			else
			{
				printf("Error: Could not open output.ppm for writing.\n");
			}

			// 3. Освобождаем указатель
			readBackBuffer_->Unmap(0, nullptr);
		}
	}

	frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
}

void Render::update(const Camera& camera, const Scene& scene, bool isDirty, float dt)
{
	SceneParameters param = calcSceneParam(width_, height_, camera.pos());
	param.primitiveCount = static_cast<uint32_t>(scene.count());
	param.trianglesCount = static_cast<uint32_t>(scene.triangles().size());
	param.sampleCount = static_cast<uint32_t>( scene.samples() );

	g_buffer.update( param );

	if (isDirty && scene.count() > 0)
	{
		// Освобождаем старый буфер
		scenePrimitives_.release();
		sceneTriangles_.release();
		createSceneSRV(scene);
	}
}


void Render::fini() 
{
	g_buffer.release();
	sceneTriangles_.release();
	scenePrimitives_.release();

	wait();
	CloseHandle(fenceEvent_);
}




