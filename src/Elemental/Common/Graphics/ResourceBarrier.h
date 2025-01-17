#pragma once

#include "../Elemental.h"
#include "SystemMemory.h"

typedef ElemHandle ResourceBarrierPool;

#define GRAPHICS_MAX_RESOURCEBARRIER 64

struct ResourceBarrierItem
{
    ElemGraphicsResourceType Type;
    bool IsDepthStencil;
    ElemGraphicsResource Resource;
    ElemGraphicsResourceBarrierSyncType BeforeSync; 
    ElemGraphicsResourceBarrierSyncType AfterSync; 
    ElemGraphicsResourceBarrierAccessType BeforeAccess;
    ElemGraphicsResourceBarrierAccessType AfterAccess;
    ElemGraphicsResourceBarrierLayoutType BeforeLayout;
    ElemGraphicsResourceBarrierLayoutType AfterLayout;
    // TODO: Add offset and size for buffer
};

struct ResourceBarriers
{
    ReadOnlySpan<ResourceBarrierItem> BufferBarriers;
    ReadOnlySpan<ResourceBarrierItem> TextureBarriers;
};

ResourceBarrierPool CreateResourceBarrierPool(MemoryArena memoryArena);
void FreeResourceBarrierPool(ResourceBarrierPool barrierPool);

void EnqueueBarrier(ResourceBarrierPool barrierPool, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options);
void EnqueueBarrier(ResourceBarrierPool barrierPool, const ResourceBarrierItem* resourceBarrier);

ResourceBarriers GenerateBarrierCommands(MemoryArena memoryArena, ResourceBarrierPool barrierPool, ElemGraphicsResourceBarrierSyncType currentStage, bool logBarrierInfo);
