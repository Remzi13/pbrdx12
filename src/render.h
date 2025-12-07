//
// Created by zakkh on 10.11.2025.
//
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h> 
#include <wrl/client.h> 

#include <d3d12.h>
#include <dxgi1_6.h>

#include <cstdint>
#include <vector>

#include "vector.h"
#include "buffers.h"
#include "scene.h"

using Microsoft::WRL::ComPtr;

struct Primitive
{
	uint32_t type;
	uint32_t matIndex;
	float radius;
	float _pad0[1];

	Vector4 position;
};

struct RTrinangle
{
	Vector3 a;
	float _padA;
	Vector3 b;
	float _padB;
	Vector3 c;
	float _padC;

	uint32_t matIndex;
	float _pad[3];
};

struct RTMaterial
{
	Vector3 albedo;
	float _pada;
	Vector3 emmision;
	float _pade;
	uint32_t type;
	float _pad[3];
};


class Render
{
public:
	bool init( HWND hwnd, const Scene& scene);
	void fini();

	void update( const Scene& scene, bool isDirty, float dt );
	void draw();

private:
	void wait();

	void createSceneSRV(const Scene& scene);

private:
	ComPtr<ID3D12Device> device_;
	ComPtr<ID3D12CommandQueue> commandQueue_;
	ComPtr<IDXGISwapChain3> swapChain_;

	ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	UINT rtvDescriptorSize_;
	
	ComPtr<ID3D12CommandAllocator> commandAllocator_;
	ComPtr<ID3D12GraphicsCommandList> commandList_;

	ComPtr<ID3D12Fence> fence_;
	UINT64 fenceValue_ = 1;
	HANDLE fenceEvent_;

	ComPtr<ID3D12RootSignature> rootSignature_; // Корневая подпись
	ComPtr<ID3D12PipelineState> pso_;         // Состояние конвейера (Pipeline State Object)
	ComPtr<ID3D12Resource> computeOutput_;      // Текстура для записи результата
	ComPtr<ID3D12DescriptorHeap> uavHeap_;      // Куча дескрипторов для UAV (Unordered Access View)

	ComPtr<ID3D12Resource> readBackBuffer_;
	UINT64 TotalBytes = 0;
	UINT64 RowSizeInBytes = 0;


	static const int FrameCount = 2;

	ComPtr<ID3D12Resource> renderTargets_[FrameCount];

	std::uint16_t width_ = 800;
	std::uint16_t height_ = 600;

	std::uint16_t frameIndex_ = 0;

	StructuredBuffer<Primitive> scenePrimitives_;
	StructuredBuffer<RTrinangle> sceneTriangles_;
	StructuredBuffer<RTMaterial> sceneMaterials_;
};

