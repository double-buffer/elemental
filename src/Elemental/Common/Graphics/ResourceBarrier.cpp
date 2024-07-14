#include "ResourceBarrier.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"
#include "SystemLogging.h"

#define GRAPHICS_MAX_RESOURCEBARRIERPOOL 64
#define GRAPHICS_MAX_RESOURCEBARRIER_RESOURCES 128

struct ResourceBarrierResourceStatus
{
    ElemGraphicsResource Resource;
    ResourceBarrierSyncType LastSyncType;
    ResourceBarrierAccessType LastAccessType;
};

struct ResourceBarrierPoolData
{
    uint32_t BarrierCount;
    ResourceBarrierItem Barriers[GRAPHICS_MAX_RESOURCEBARRIER];

    // TODO: Review that later, for the moment we do a linear search on the resource because per command list
    // we shouldn't have too much associated resources that have barriers
    uint32_t ResourceStatusCount;
    ResourceBarrierResourceStatus ResourceStatus[GRAPHICS_MAX_RESOURCEBARRIER_RESOURCES];
};

SystemDataPool<ResourceBarrierPoolData, SystemDataPoolDefaultFull> resourceBarrierDataPool;

void InitResourceBarrierPoolMemory(MemoryArena memoryArena)
{
    if (!resourceBarrierDataPool.Storage)
    {
        resourceBarrierDataPool = SystemCreateDataPool<ResourceBarrierPoolData>(memoryArena, GRAPHICS_MAX_RESOURCEBARRIERPOOL);
    }
}

ReadOnlySpan<char> ResourceBarrierSyncTypeToString(MemoryArena memoryArena, ResourceBarrierSyncType syncType)
{
    switch (syncType) 
    {
        case SyncType_None: return SystemDuplicateBuffer<char>(memoryArena, "None"); 
        case SyncType_Compute: return SystemDuplicateBuffer<char>(memoryArena, "Compute");
        default: return SystemDuplicateBuffer<char>(memoryArena, "Unknown"); 
    }
}

ReadOnlySpan<char> ResourceBarrierAccessTypeToString(MemoryArena memoryArena, ResourceBarrierAccessType accessType)
{
    switch (accessType) 
    {
        case AccessType_NoAccess: return SystemDuplicateBuffer<char>(memoryArena, "NoAccess"); 
        case AccessType_Read: return SystemDuplicateBuffer<char>(memoryArena, "Read");
        case AccessType_Write: return SystemDuplicateBuffer<char>(memoryArena, "Write");
        default: return SystemDuplicateBuffer<char>(memoryArena, "Unknown"); 
    }
}

ResourceBarrierResourceStatus* GetResourceBarrierResourceStatus(ResourceBarrierPoolData* poolData, ElemGraphicsResource resource)
{
    for (uint32_t i = 0; i < poolData->ResourceStatusCount; i++)
    {
        if (poolData->ResourceStatus[i].Resource == resource)
        {
            return &poolData->ResourceStatus[i];
        }
    }

    return nullptr;
}

ResourceBarrierPool CreateResourceBarrierPool(MemoryArena memoryArena)
{
    InitResourceBarrierPoolMemory(memoryArena);

    return SystemAddDataPoolItem(resourceBarrierDataPool, {
        .BarrierCount = 0
    });
}

void FreeResourceBarrierPool(ResourceBarrierPool barrierPool)
{
    SystemRemoveDataPoolItem(resourceBarrierDataPool, barrierPool);
}

void EnqueueBarrier(ResourceBarrierPool barrierPool, const ResourceBarrierItem* resourceBarrier)
{
    SystemAssert(barrierPool != ELEM_HANDLE_NULL);

    auto barrierPoolData = SystemGetDataPoolItem(resourceBarrierDataPool, barrierPool);
    SystemAssert(barrierPoolData);

    barrierPoolData->Barriers[barrierPoolData->BarrierCount++] = *resourceBarrier;
}

ResourceBarriers GenerateBarrierCommands(MemoryArena memoryArena, ResourceBarrierPool barrierPool, ResourceBarrierSyncType currentStage, bool logBarrierInfo)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    SystemAssert(barrierPool != ELEM_HANDLE_NULL);

    auto barrierPoolData = SystemGetDataPoolItem(resourceBarrierDataPool, barrierPool);
    SystemAssert(barrierPoolData);

    // TODO: Merge similar barriers on the same resource

    auto bufferBarrierCount = 0u;
    auto bufferBarriers = SystemPushArray<ResourceBarrierItem>(memoryArena, GRAPHICS_MAX_RESOURCEBARRIER);

    for (uint32_t i = 0; i < barrierPoolData->BarrierCount; i++)
    {
        auto barrierItem = barrierPoolData->Barriers[i];
        auto resourceStatus = GetResourceBarrierResourceStatus(barrierPoolData, barrierItem.Resource);

        if (barrierItem.Type == ElemGraphicsResourceType_Buffer)
        {
            if (resourceStatus)
            {
                if (barrierItem.SyncBefore == SyncType_None && resourceStatus->LastSyncType != SyncType_None)
                {
                    barrierItem.SyncBefore = resourceStatus->LastSyncType;
                }
                
                if (barrierItem.AccessBefore == AccessType_NoAccess && resourceStatus->LastAccessType != AccessType_NoAccess)
                {
                    barrierItem.AccessBefore = resourceStatus->LastAccessType;
                }
            }

            if (barrierItem.SyncAfter == SyncType_None)
            {
                barrierItem.SyncAfter = currentStage;
            }

            bufferBarriers[bufferBarrierCount++] = barrierItem;
        }

        if (!resourceStatus)
        {
            resourceStatus = &barrierPoolData->ResourceStatus[barrierPoolData->ResourceStatusCount++];

            *resourceStatus = 
            {
                .Resource = barrierItem.Resource,
            };
        }

        resourceStatus->LastSyncType = currentStage;
        resourceStatus->LastAccessType = barrierItem.AccessAfter;
    }

    if (logBarrierInfo)
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "BarrierCommand: Buffer=%d, Texture=0", bufferBarrierCount);

        for (uint32_t i = 0; i < bufferBarrierCount; i++)
        {
            auto barrierItem = bufferBarriers[i];
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "  BarrierBuffer: Resource=%d, SyncBefore=%s, SyncAfter=%s, AccessBefore=%s, AccessAfter=%s", 
                                    barrierItem.Resource,
                                    ResourceBarrierSyncTypeToString(stackMemoryArena, barrierItem.SyncBefore).Pointer,
                                    ResourceBarrierSyncTypeToString(stackMemoryArena, barrierItem.SyncAfter).Pointer,
                                    ResourceBarrierAccessTypeToString(stackMemoryArena, barrierItem.AccessBefore).Pointer,
                                    ResourceBarrierAccessTypeToString(stackMemoryArena, barrierItem.AccessAfter).Pointer);
        }
        //SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "  TextureBuffer: Resource: 1, SyncBefore=Compute, SyncAfter=Compute, AccessBefore=Read, AccessAfter=Write, LayoutBefore=Read, LayoutAfter=RenderTarget");
    }

    barrierPoolData->BarrierCount = 0;

    return 
    {
        .BufferBarriers = bufferBarriers.Slice(0, bufferBarrierCount)
    };
}
