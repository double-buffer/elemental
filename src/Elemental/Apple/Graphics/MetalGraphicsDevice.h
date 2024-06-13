#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

#define METAL_MAX_RESOURCES 1000000

struct MetalArgumentBufferStorage;

struct MetalArgumentBuffer
{
    MetalArgumentBufferStorage* Storage;
};

struct MetalGraphicsDeviceData
{
    NS::SharedPtr<MTL::Device> Device;
    MetalArgumentBuffer ResourceArgumentBuffer;
};

struct MetalGraphicsDeviceDataFull
{
    NS::SharedPtr<MTL::ResidencySet> ResidencySet;
};

extern MemoryArena MetalGraphicsMemoryArena;
extern bool MetalDebugLayerEnabled;

MetalGraphicsDeviceData* GetMetalGraphicsDeviceData(ElemGraphicsDevice graphicsDevice);
MetalGraphicsDeviceDataFull* GetMetalGraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice);

uint32_t CreateMetalArgumentBufferTextureHandle(MetalArgumentBuffer argumentBuffer, MTL::Texture* texture);
void FreeMetalArgumentBufferHandle(MetalArgumentBuffer argumentBuffer, uint64_t handle);

void MetalEnableGraphicsDebugLayer();
ElemGraphicsDeviceInfoSpan MetalGetAvailableGraphicsDevices();
ElemGraphicsDevice MetalCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
void MetalFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemGraphicsDeviceInfo MetalGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);
