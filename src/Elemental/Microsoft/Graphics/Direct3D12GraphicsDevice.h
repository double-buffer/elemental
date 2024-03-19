#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

struct Direct3D12GraphicsDeviceData
{
    ComPtr<ID3D12Device10> Device;
};

struct Direct3D12GraphicsDeviceDataFull
{
    DXGI_ADAPTER_DESC3 AdapterDescription;
    ComPtr<ID3D12InfoQueue1> DebugInfoQueue;
};

extern MemoryArena Direct3D12MemoryArena;
extern bool Direct3D12DebugLayerEnabled;

Direct3D12GraphicsDeviceData* GetDirect3D12GraphicsDeviceData(ElemGraphicsDevice graphicsDevice);
Direct3D12GraphicsDeviceDataFull* GetDirect3D12GraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice);

void Direct3D12EnableGraphicsDebugLayer();
ElemGraphicsDeviceInfoSpan Direct3D12GetAvailableGraphicsDevices();
ElemGraphicsDevice Direct3D12CreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
void Direct3D12FreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemGraphicsDeviceInfo Direct3D12GetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);
