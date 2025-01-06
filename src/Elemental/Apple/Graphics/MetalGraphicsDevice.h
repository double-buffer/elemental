#pragma once

#include "Graphics/UploadBufferPool.h"
#include "Elemental.h"
#include "SystemMemory.h"

#define METAL_MAX_DEVICES 10u
// TODO: Increase to 1.000.000?
#define METAL_MAX_RESOURCES 500000

struct MetalArgumentBufferStorage;

struct MetalArgumentBuffer
{
    MetalArgumentBufferStorage* Storage;
};

struct MetalGraphicsDeviceData
{
    NS::SharedPtr<MTL::Device> Device;
    MetalArgumentBuffer ResourceArgumentBuffer;
    MemoryArena MemoryArena;
    uint64_t UploadBufferGeneration;
    Span<UploadBufferDevicePool<NS::SharedPtr<MTL::Buffer>>*> UploadBufferPools;
    uint32_t CurrentUploadBufferPoolIndex;
};

struct MetalGraphicsDeviceDataFull
{
    NS::SharedPtr<MTL::ResidencySet> ResidencySet;
};

extern MemoryArena MetalGraphicsMemoryArena;
extern bool MetalDebugLayerEnabled;
extern bool MetalDebugBarrierInfoEnabled;

MetalGraphicsDeviceData* GetMetalGraphicsDeviceData(ElemGraphicsDevice graphicsDevice);
MetalGraphicsDeviceDataFull* GetMetalGraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice);

uint32_t CreateMetalArgumentBufferHandleForTexture(MetalArgumentBuffer argumentBuffer, MTL::Texture* texture);
uint32_t CreateMetalArgumentBufferHandleForBuffer(MetalArgumentBuffer argumentBuffer, MTL::Buffer* buffer, uint32_t length);
void FreeMetalArgumentBufferHandle(MetalArgumentBuffer argumentBuffer, uint64_t handle);

void MetalSetGraphicsOptions(const ElemGraphicsOptions* options);

ElemGraphicsDeviceInfoSpan MetalGetAvailableGraphicsDevices();
ElemGraphicsDevice MetalCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
void MetalFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemGraphicsDeviceInfo MetalGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);
