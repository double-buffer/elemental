#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

struct MetalGraphicsDeviceData
{
    NS::SharedPtr<MTL::Device> Device;
};

struct MetalGraphicsDeviceDataFull
{
    uint32_t Reserved;
};

extern MemoryArena MetalGraphicsMemoryArena;
extern bool MetalDebugLayerEnabled;

MetalGraphicsDeviceData* GetMetalGraphicsDeviceData(ElemGraphicsDevice graphicsDevice);
MetalGraphicsDeviceDataFull* GetMetalGraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice);

void MetalEnableGraphicsDebugLayer();
ElemGraphicsDeviceInfoSpan MetalGetAvailableGraphicsDevices();
ElemGraphicsDevice MetalCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
void MetalFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemGraphicsDeviceInfo MetalGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);
