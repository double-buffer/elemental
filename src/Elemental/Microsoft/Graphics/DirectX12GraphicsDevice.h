#pragma once

#include "Elemental.h"
#include "Graphics/UploadBufferPool.h"
#include "SystemMemory.h"

#define DIRECTX12_MAX_DEVICES 10u

// TODO: Put this value a a config? 
#define DIRECTX12_MAX_RESOURCES 1000000

struct DirectX12DescriptorHeapStorage;

struct DirectX12DescriptorHeap
{
    DirectX12DescriptorHeapStorage* Storage;
};

struct DirectX12GraphicsDeviceData
{
    ComPtr<ID3D12Device10> Device;
    ComPtr<ID3D12RootSignature> RootSignature;
    uint64_t CommandAllocationGeneration;
    uint64_t UploadBufferGeneration;
    DirectX12DescriptorHeap ResourceDescriptorHeap;
    DirectX12DescriptorHeap RTVDescriptorHeap;
    DirectX12DescriptorHeap DSVDescriptorHeap;
    MemoryArena MemoryArena;
    Span<UploadBufferDevicePool<ComPtr<ID3D12Resource>>*> UploadBufferPools;
    uint32_t CurrentUploadBufferPoolIndex;
};

struct DirectX12GraphicsDeviceDataFull
{
    DXGI_ADAPTER_DESC3 AdapterDescription;
    ComPtr<ID3D12InfoQueue1> DebugInfoQueue;
    DWORD DebugCallBackCookie;
};

extern MemoryArena DirectX12MemoryArena;
extern bool DirectX12DebugLayerEnabled;
extern bool DirectX12DebugBarrierInfoEnabled;
extern ComPtr<IDXGIFactory6> DxgiFactory; 
extern ComPtr<IDXGIInfoQueue> DxgiInfoQueue;

DirectX12GraphicsDeviceData* GetDirectX12GraphicsDeviceData(ElemGraphicsDevice graphicsDevice);
DirectX12GraphicsDeviceDataFull* GetDirectX12GraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice);

D3D12_CPU_DESCRIPTOR_HANDLE CreateDirectX12DescriptorHandle(DirectX12DescriptorHeap descriptorHeap);
void FreeDirectX12DescriptorHandle(DirectX12DescriptorHeap descriptorHeap, D3D12_CPU_DESCRIPTOR_HANDLE handle);
uint32_t ConvertDirectX12DescriptorHandleToIndex(DirectX12DescriptorHeap descriptorHeap, D3D12_CPU_DESCRIPTOR_HANDLE handle);
D3D12_CPU_DESCRIPTOR_HANDLE ConvertDirectX12DescriptorIndexToHandle(DirectX12DescriptorHeap descriptorHeap, uint32_t index);

void DirectX12SetGraphicsOptions(const ElemGraphicsOptions* options);

ElemGraphicsDeviceInfoSpan DirectX12GetAvailableGraphicsDevices();
ElemGraphicsDevice DirectX12CreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
void DirectX12FreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemGraphicsDeviceInfo DirectX12GetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);
