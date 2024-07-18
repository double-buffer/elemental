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
    ResourceBarrierLayoutType LastLayoutType;
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

ReadOnlySpan<char> ResourceBarrierLayoutTypeToString(MemoryArena memoryArena, ResourceBarrierLayoutType layoutType)
{
    switch (layoutType) 
    {
        case LayoutType_Read: return SystemDuplicateBuffer<char>(memoryArena, "Read");
        case LayoutType_Write: return SystemDuplicateBuffer<char>(memoryArena, "Write");
        case LayoutType_RenderTarget: return SystemDuplicateBuffer<char>(memoryArena, "RenderTarget");
        default: return SystemDuplicateBuffer<char>(memoryArena, "Undefined"); 
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
    
    auto textureBarrierCount = 0u;
    auto textureBarriers = SystemPushArray<ResourceBarrierItem>(memoryArena, GRAPHICS_MAX_RESOURCEBARRIER);

    for (uint32_t i = 0; i < barrierPoolData->BarrierCount; i++)
    {
        auto barrierItem = barrierPoolData->Barriers[i];
        auto resourceStatus = GetResourceBarrierResourceStatus(barrierPoolData, barrierItem.Resource);

        if (barrierItem.Type == ElemGraphicsResourceType_Buffer)
        {
            // TODO: Process status in a separate functions?
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
        else
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

                if (barrierItem.LayoutBefore == LayoutType_Undefined && resourceStatus->LastLayoutType != LayoutType_Undefined)
                {
                    barrierItem.LayoutBefore = resourceStatus->LastLayoutType;
                }
            }

            if (barrierItem.SyncAfter == SyncType_None)
            {
                barrierItem.SyncAfter = currentStage;
            }

            textureBarriers[textureBarrierCount++] = barrierItem;
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
        resourceStatus->LastLayoutType = barrierItem.LayoutAfter;
    }
    
    if (logBarrierInfo && (bufferBarrierCount > 0 || textureBarrierCount > 0))
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "BarrierCommand: Buffer=%d, Texture=%d", bufferBarrierCount, textureBarrierCount);

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

        for (uint32_t i = 0; i < textureBarrierCount; i++)
        {
            auto barrierItem = textureBarriers[i];
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "  BarrierTexture: Resource=%d, SyncBefore=%s, SyncAfter=%s, AccessBefore=%s, AccessAfter=%s, LayoutBefore=%s, LayoutAfter=%s", 
                                    barrierItem.Resource,
                                    ResourceBarrierSyncTypeToString(stackMemoryArena, barrierItem.SyncBefore).Pointer,
                                    ResourceBarrierSyncTypeToString(stackMemoryArena, barrierItem.SyncAfter).Pointer,
                                    ResourceBarrierAccessTypeToString(stackMemoryArena, barrierItem.AccessBefore).Pointer,
                                    ResourceBarrierAccessTypeToString(stackMemoryArena, barrierItem.AccessAfter).Pointer,
                                    ResourceBarrierLayoutTypeToString(stackMemoryArena, barrierItem.LayoutBefore).Pointer,
                                    ResourceBarrierLayoutTypeToString(stackMemoryArena, barrierItem.LayoutAfter).Pointer);
        }
    }

    barrierPoolData->BarrierCount = 0;

    return 
    {
        .BufferBarriers = bufferBarriers.Slice(0, bufferBarrierCount),
        .TextureBarriers = textureBarriers.Slice(0, textureBarrierCount)
    };
}
