#pragma once

#include "../Elemental.h"
#include "SystemMemory.h"

typedef ElemHandle ResourceBarrierPool;

#define GRAPHICS_MAX_RESOURCEBARRIER 64

// TODO: Normally we can remove this because we will have the enums in the main lib
enum ResourceBarrierSyncType
{
    SyncType_None,
    SyncType_Compute
};

enum ResourceBarrierAccessType
{
    AccessType_NoAccess,
    AccessType_Read,
    AccessType_Write
};

enum ResourceBarrierLayoutType
{
    LayoutType_Undefined,
    LayoutType_Read,
    LayoutType_Write,
    LayoutType_RenderTarget
};

struct ResourceBarrierItem
{
    ElemGraphicsResourceType Type;
    ElemGraphicsResource Resource;
    ResourceBarrierSyncType SyncBefore; 
    ResourceBarrierSyncType SyncAfter; 
    ResourceBarrierAccessType AccessBefore;
    ResourceBarrierAccessType AccessAfter;
    ResourceBarrierLayoutType LayoutBefore;
    ResourceBarrierLayoutType LayoutAfter;
};

struct ResourceBarriers
{
    ReadOnlySpan<ResourceBarrierItem> BufferBarriers;
    ReadOnlySpan<ResourceBarrierItem> TextureBarriers;
};

ResourceBarrierPool CreateResourceBarrierPool(MemoryArena memoryArena);
void FreeResourceBarrierPool(ResourceBarrierPool barrierPool);

void EnqueueBarrier(ResourceBarrierPool barrierPool, const ResourceBarrierItem* resourceBarrier);
ResourceBarriers GenerateBarrierCommands(MemoryArena memoryArena, ResourceBarrierPool barrierPool, ResourceBarrierSyncType currentStage, bool logBarrierInfo);
