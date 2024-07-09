#pragma once

#include "Elemental.h"
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

struct ResourceBarrierItem
{
    ElemGraphicsResourceType Type;
    ElemGraphicsResource Resource;
    ResourceBarrierSyncType SyncBefore; 
    ResourceBarrierSyncType SyncAfter; 
    ResourceBarrierAccessType AccessBefore;
    ResourceBarrierAccessType AccessAfter;
    // TODO: Layouts
};

struct ResourceBarriers
{
    ReadOnlySpan<ResourceBarrierItem> BufferBarriers;
    ReadOnlySpan<ResourceBarrierItem> TextureBarriers;
};

struct ResourceBarrierPoolData
{
    uint32_t BarrierCount;
    ResourceBarrierItem Barriers[GRAPHICS_MAX_RESOURCEBARRIER];

    // TODO: Last sync and access should be stored per resources
    ResourceBarrierSyncType LastSyncType;
    ResourceBarrierAccessType LastAccessType;
};

ResourceBarrierPool CreateResourceBarrierPool(MemoryArena memoryArena);
void FreeResourceBarrierPool(ResourceBarrierPool barrierPool);

void EnqueueBarrier(ResourceBarrierPool barrierPool, const ResourceBarrierItem* resourceBarrier);
ResourceBarriers GenerateBarrierCommands(MemoryArena memoryArena, ResourceBarrierPool barrierPool, ResourceBarrierSyncType currentStage, bool logBarrierInfo);
