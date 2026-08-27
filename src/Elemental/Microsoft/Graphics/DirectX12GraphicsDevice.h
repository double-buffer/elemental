#pragma once

#include "Elemental.h"
#include "Graphics/UploadBufferPool.h"
#include "SystemMemory.h"

struct DirectX12FreeListItem
{
    uint32_t Next;
};

struct DirectX12DescriptorHeapStorage
{
    ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
    Span<DirectX12FreeListItem> Items;
    uint32_t DescriptorHandleSize;
    uint32_t CurrentIndex;
    uint32_t FreeListIndex;
};

struct DirectX12DescriptorHeap
{
    DirectX12DescriptorHeapStorage* Storage;
};

struct DirectX12QueryHeapStorage
{
    ComPtr<ID3D12QueryHeap> QueryHeap;
    ComPtr<ID3D12Resource> QueryHeapReadbackBuffer;
    D3D12_QUERY_HEAP_TYPE Type;
    void* ReadbackCpuPointer;
    Span<DirectX12FreeListItem> Items;
    uint32_t CurrentIndex;
    uint32_t InitializationIndex;
    uint32_t FreeListIndex;
};

struct DirectX12QueryHeap
{
    DirectX12QueryHeapStorage* Storage;
};

struct DirectX12GraphicsDeviceData
{
    ComPtr<ID3D12Device10> Device;
    ComPtr<ID3D12RootSignature> RootSignature;
    uint64_t CommandAllocationGeneration;
    uint64_t UploadBufferGeneration;
    DirectX12DescriptorHeap ResourceDescriptorHeap;
    DirectX12DescriptorHeap SamplerDescriptorHeap;
    DirectX12DescriptorHeap RTVDescriptorHeap;
    DirectX12DescriptorHeap DSVDescriptorHeap;
    MemoryArena MemoryArena;
    Span<UploadBufferDevicePool<ComPtr<ID3D12Resource>>*> UploadBufferPools;
    uint32_t CurrentUploadBufferPoolIndex;
    DirectX12QueryHeap QueryHeap;
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

D3D12_COMPARISON_FUNC ConvertToDirectX12CompareFunction(ElemGraphicsCompareFunction compareFunction);

D3D12_CPU_DESCRIPTOR_HANDLE CreateDirectX12DescriptorHandle(DirectX12DescriptorHeap descriptorHeap);
void FreeDirectX12DescriptorHandle(DirectX12DescriptorHeap descriptorHeap, D3D12_CPU_DESCRIPTOR_HANDLE handle);
uint32_t ConvertDirectX12DescriptorHandleToIndex(DirectX12DescriptorHeap descriptorHeap, D3D12_CPU_DESCRIPTOR_HANDLE handle);
D3D12_CPU_DESCRIPTOR_HANDLE ConvertDirectX12DescriptorIndexToHandle(DirectX12DescriptorHeap descriptorHeap, uint32_t index);

uint32_t CreateDirectX12QueryHeapIndex(DirectX12QueryHeap queryHeap);
void FreeDirectX12QueryHeapIndex(DirectX12QueryHeap queryHeap, uint32_t index);

void DirectX12SetGraphicsOptions(const ElemGraphicsOptions* options);

ElemGraphicsDeviceInfoSpan DirectX12GetAvailableGraphicsDevices();
ElemGraphicsDevice DirectX12CreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
void DirectX12FreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemGraphicsDeviceInfo DirectX12GetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);
