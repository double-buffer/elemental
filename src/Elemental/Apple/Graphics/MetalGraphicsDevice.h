#pragma once

#include "Graphics/UploadBufferPool.h"
#include "Elemental.h"
#include "SystemMemory.h"

struct MetalArgumentBufferStorage;

struct MetalArgumentBuffer
{
    MetalArgumentBufferStorage* Storage;
};

struct MetalGraphicsDeviceData
{
    NS::SharedPtr<MTL::Device> Device;
    MetalArgumentBuffer ResourceArgumentBuffer;
    MetalArgumentBuffer SamplerArgumentBuffer;
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

MTL::CompareFunction ConvertToMetalCompareFunction(ElemGraphicsCompareFunction compareFunction);

uint32_t CreateMetalArgumentBufferHandleForTexture(MetalArgumentBuffer argumentBuffer, MTL::Texture* texture);
uint32_t CreateMetalArgumentBufferHandleForBuffer(MetalArgumentBuffer argumentBuffer, MTL::Buffer* buffer, uint32_t length);
uint32_t CreateMetalArgumentBufferHandleForSamplerState(MetalArgumentBuffer argumentBuffer, MTL::SamplerState* sampler);
void FreeMetalArgumentBufferHandle(MetalArgumentBuffer argumentBuffer, uint64_t handle);

void MetalSetGraphicsOptions(const ElemGraphicsOptions* options);

ElemGraphicsDeviceInfoSpan MetalGetAvailableGraphicsDevices();
ElemGraphicsDevice MetalCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
void MetalFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemGraphicsDeviceInfo MetalGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);
