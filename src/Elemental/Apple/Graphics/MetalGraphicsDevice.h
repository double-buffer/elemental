#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

struct MetalGraphicsDeviceData
{
};

struct MetalGraphicsDeviceDataFull
{
};

extern MemoryArena MetalGraphicsMemoryArena;

MetalGraphicsDeviceData* GetMetalGraphicsDeviceData(ElemGraphicsDevice graphicsDevice);
MetalGraphicsDeviceDataFull* GetMetalGraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice);

void MetalEnableGraphicsDebugLayer();
ElemGraphicsDeviceInfoList MetalGetAvailableGraphicsDevices();
ElemGraphicsDevice MetalCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
void MetalFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemGraphicsDeviceInfo MetalGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);
