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
    ElemGraphicsResourceBarrierSyncType LastSyncType;
    ElemGraphicsResourceBarrierAccessType LastAccessType;
    ElemGraphicsResourceBarrierLayoutType LastLayoutType;
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

ReadOnlySpan<char> ResourceBarrierSyncTypeToString(MemoryArena memoryArena, ElemGraphicsResourceBarrierSyncType syncType)
{
    switch (syncType) 
    {
        case ElemGraphicsResourceBarrierSyncType_None: return SystemDuplicateBuffer<char>(memoryArena, "None"); 
        case ElemGraphicsResourceBarrierSyncType_Compute: return SystemDuplicateBuffer<char>(memoryArena, "Compute");
        case ElemGraphicsResourceBarrierSyncType_RenderTarget: return SystemDuplicateBuffer<char>(memoryArena, "RenderTarget");
        default: return SystemDuplicateBuffer<char>(memoryArena, "Unknown"); 
    }
}

ReadOnlySpan<char> ResourceBarrierAccessTypeToString(MemoryArena memoryArena, ElemGraphicsResourceBarrierAccessType accessType)
{
    switch (accessType) 
    {
        case ElemGraphicsResourceBarrierAccessType_NoAccess: return SystemDuplicateBuffer<char>(memoryArena, "NoAccess"); 
        case ElemGraphicsResourceBarrierAccessType_Read: return SystemDuplicateBuffer<char>(memoryArena, "Read");
        case ElemGraphicsResourceBarrierAccessType_Write: return SystemDuplicateBuffer<char>(memoryArena, "Write");
        case ElemGraphicsResourceBarrierAccessType_RenderTarget: return SystemDuplicateBuffer<char>(memoryArena, "RenderTarget");
        default: return SystemDuplicateBuffer<char>(memoryArena, "Unknown"); 
    }
}

ReadOnlySpan<char> ResourceBarrierLayoutTypeToString(MemoryArena memoryArena, ElemGraphicsResourceBarrierLayoutType layoutType)
{
    switch (layoutType) 
    {
        case ElemGraphicsResourceBarrierLayoutType_Read: return SystemDuplicateBuffer<char>(memoryArena, "Read");
        case ElemGraphicsResourceBarrierLayoutType_Write: return SystemDuplicateBuffer<char>(memoryArena, "Write");
        case ElemGraphicsResourceBarrierLayoutType_RenderTarget: return SystemDuplicateBuffer<char>(memoryArena, "RenderTarget");
        case ElemGraphicsResourceBarrierLayoutType_Present: return SystemDuplicateBuffer<char>(memoryArena, "Present");
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

void EnqueueBarrier(ResourceBarrierPool barrierPool, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options)
{
    auto descriptorInfo = ElemGetGraphicsResourceDescriptorInfo(descriptor);
    auto resourceInfo = ElemGetGraphicsResourceInfo(descriptorInfo.Resource);
    
    ResourceBarrierItem resourceBarrier =
    {
        .Type = resourceInfo.Type,
        .Resource = descriptorInfo.Resource,
        .AfterAccess = (descriptorInfo.Usage & ElemGraphicsResourceDescriptorUsage_Write) ? ElemGraphicsResourceBarrierAccessType_Write : ElemGraphicsResourceBarrierAccessType_Read,
        .AfterLayout = (descriptorInfo.Usage & ElemGraphicsResourceDescriptorUsage_Write) ? ElemGraphicsResourceBarrierLayoutType_Write : ElemGraphicsResourceBarrierLayoutType_Read
    };

    if (options)
    {
        if (options->BeforeSync != ElemGraphicsResourceBarrierSyncType_None)
        {
            resourceBarrier.BeforeSync = options->BeforeSync;
        }

        if (options->AfterSync != ElemGraphicsResourceBarrierSyncType_None)
        {
            resourceBarrier.AfterSync = options->AfterSync;
        }

        if (options->BeforeAccess != ElemGraphicsResourceBarrierAccessType_NoAccess)
        {
            resourceBarrier.BeforeAccess = options->BeforeAccess;
        }

        if (options->AfterAccess != ElemGraphicsResourceBarrierAccessType_NoAccess)
        {
            resourceBarrier.AfterAccess = options->AfterAccess;
        }

        if (options->BeforeLayout != ElemGraphicsResourceBarrierLayoutType_Undefined)
        {
            resourceBarrier.BeforeLayout = options->BeforeLayout;
        }

        if (options->AfterLayout != ElemGraphicsResourceBarrierLayoutType_Undefined)
        {
            resourceBarrier.AfterLayout = options->AfterLayout;
        }
    }

    EnqueueBarrier(barrierPool, &resourceBarrier);
}

void EnqueueBarrier(ResourceBarrierPool barrierPool, const ResourceBarrierItem* resourceBarrier)
{
    SystemAssert(barrierPool != ELEM_HANDLE_NULL);

    auto barrierPoolData = SystemGetDataPoolItem(resourceBarrierDataPool, barrierPool);
    SystemAssert(barrierPoolData);
  
    barrierPoolData->Barriers[barrierPoolData->BarrierCount++] = *resourceBarrier;
}

ResourceBarriers GenerateBarrierCommands(MemoryArena memoryArena, ResourceBarrierPool barrierPool, ElemGraphicsResourceBarrierSyncType currentStage, bool logBarrierInfo)
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
                if (barrierItem.BeforeSync == ElemGraphicsResourceBarrierSyncType_None && resourceStatus->LastSyncType != ElemGraphicsResourceBarrierSyncType_None)
                {
                    barrierItem.BeforeSync = resourceStatus->LastSyncType;
                }
                
                if (barrierItem.BeforeAccess == ElemGraphicsResourceBarrierAccessType_NoAccess && resourceStatus->LastAccessType != ElemGraphicsResourceBarrierAccessType_NoAccess)
                {
                    barrierItem.BeforeAccess = resourceStatus->LastAccessType;
                }
            }

            if (barrierItem.AfterSync == ElemGraphicsResourceBarrierSyncType_None)
            {
                barrierItem.AfterSync = currentStage;
            }

            bufferBarriers[bufferBarrierCount++] = barrierItem;
        }
        else
        {
            if (resourceStatus)
            {
                if (barrierItem.BeforeSync == ElemGraphicsResourceBarrierSyncType_None && resourceStatus->LastSyncType != ElemGraphicsResourceBarrierSyncType_None)
                {
                    barrierItem.BeforeSync = resourceStatus->LastSyncType;
                }
                
                if (barrierItem.BeforeAccess == ElemGraphicsResourceBarrierAccessType_NoAccess && resourceStatus->LastAccessType != ElemGraphicsResourceBarrierAccessType_NoAccess)
                {
                    barrierItem.BeforeAccess = resourceStatus->LastAccessType;
                }

                if (barrierItem.BeforeLayout == ElemGraphicsResourceBarrierLayoutType_Undefined && resourceStatus->LastLayoutType != ElemGraphicsResourceBarrierLayoutType_Undefined)
                {
                    barrierItem.BeforeLayout = resourceStatus->LastLayoutType;
                }
            }

            if (barrierItem.AfterSync == ElemGraphicsResourceBarrierSyncType_None)
            {
                barrierItem.AfterSync = currentStage;
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
        resourceStatus->LastAccessType = barrierItem.AfterAccess;
        resourceStatus->LastLayoutType = barrierItem.AfterLayout;
    }
    
    if (logBarrierInfo && (bufferBarrierCount > 0 || textureBarrierCount > 0))
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "BarrierCommand: Buffer=%d, Texture=%d", bufferBarrierCount, textureBarrierCount);

        for (uint32_t i = 0; i < bufferBarrierCount; i++)
        {
            auto barrierItem = bufferBarriers[i];
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "  BarrierBuffer: Resource=%d, SyncBefore=%s, SyncAfter=%s, AccessBefore=%s, AccessAfter=%s", 
                                    barrierItem.Resource,
                                    ResourceBarrierSyncTypeToString(stackMemoryArena, barrierItem.BeforeSync).Pointer,
                                    ResourceBarrierSyncTypeToString(stackMemoryArena, barrierItem.AfterSync).Pointer,
                                    ResourceBarrierAccessTypeToString(stackMemoryArena, barrierItem.BeforeAccess).Pointer,
                                    ResourceBarrierAccessTypeToString(stackMemoryArena, barrierItem.AfterAccess).Pointer);
        }

        for (uint32_t i = 0; i < textureBarrierCount; i++)
        {
            auto barrierItem = textureBarriers[i];
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "  BarrierTexture: Resource=%d, SyncBefore=%s, SyncAfter=%s, AccessBefore=%s, AccessAfter=%s, LayoutBefore=%s, LayoutAfter=%s", 
                                    barrierItem.Resource,
                                    ResourceBarrierSyncTypeToString(stackMemoryArena, barrierItem.BeforeSync).Pointer,
                                    ResourceBarrierSyncTypeToString(stackMemoryArena, barrierItem.AfterSync).Pointer,
                                    ResourceBarrierAccessTypeToString(stackMemoryArena, barrierItem.BeforeAccess).Pointer,
                                    ResourceBarrierAccessTypeToString(stackMemoryArena, barrierItem.AfterAccess).Pointer,
                                    ResourceBarrierLayoutTypeToString(stackMemoryArena, barrierItem.BeforeLayout).Pointer,
                                    ResourceBarrierLayoutTypeToString(stackMemoryArena, barrierItem.AfterLayout).Pointer);
        }
    }

    barrierPoolData->BarrierCount = 0;

    return 
    {
        .BufferBarriers = bufferBarriers.Slice(0, bufferBarrierCount),
        .TextureBarriers = textureBarriers.Slice(0, textureBarrierCount)
    };
}
