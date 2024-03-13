#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

struct Direct3D12GraphicsDeviceData
{
};

struct Direct3D12GraphicsDeviceDataFull
{

};

extern MemoryArena Direct3D12GraphicsMemoryArena;

Direct3D12GraphicsDeviceData* GetDirect3D12GraphicsDeviceData(ElemGraphicsDevice graphicsDevice);
Direct3D12GraphicsDeviceDataFull* GetDirect3D12GraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice);

void Direct3D12EnableGraphicsDebugLayer();
ElemGraphicsDeviceInfoList Direct3D12GetAvailableGraphicsDevices();
