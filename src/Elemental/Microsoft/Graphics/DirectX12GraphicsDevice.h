#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

struct DirectX12GraphicsDeviceData
{
    ComPtr<ID3D12Device10> Device;
};

struct DirectX12GraphicsDeviceDataFull
{
    DXGI_ADAPTER_DESC3 AdapterDescription;
    ComPtr<ID3D12InfoQueue1> DebugInfoQueue;
};

extern MemoryArena DirectX12MemoryArena;
extern bool DirectX12DebugLayerEnabled;

DirectX12GraphicsDeviceData* GetDirectX12GraphicsDeviceData(ElemGraphicsDevice graphicsDevice);
DirectX12GraphicsDeviceDataFull* GetDirectX12GraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice);

void DirectX12EnableGraphicsDebugLayer();
ElemGraphicsDeviceInfoSpan DirectX12GetAvailableGraphicsDevices();
ElemGraphicsDevice DirectX12CreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
void DirectX12FreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemGraphicsDeviceInfo DirectX12GetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);
