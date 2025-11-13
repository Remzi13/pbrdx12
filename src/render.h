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

using Microsoft::WRL::ComPtr;

struct Primitive
{
	uint32_t type;
	float radius;
	float _pad0[2];

	Vector4 position_point; // X, Y, Z, W(игнорируется)
	Vector4 normal_color;   // X, Y, Z, W(игнорируется)
};

class Camera
{
public:
	const Vector3& pos() const
	{
		return pos_;
	}

	void setPos( const Vector3& pos )
	{
		pos_ = pos;
	}

private:
	Vector3 pos_;
};

class Render
{
public:
	bool init( HWND hwnd, std::uint16_t width, std::uint16_t height );
	void fini();

	void update( const Camera& camera, const std::vector<Primitive>& scene, bool isDirty, float dt );
	void draw();

private:
	void wait();

	void createSceneSRV();

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

	StructuredBuffer<Primitive> sceneBuffer_;
};

