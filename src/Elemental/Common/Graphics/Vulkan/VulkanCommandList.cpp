#include "VulkanCommandList.h"
#include "VulkanGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define VULKAN_MAX_COMMANDQUEUES 10u
#define VULKAN_MAX_COMMANDLISTS 64u

SystemDataPool<VulkanCommandQueueData, VulkanCommandQueueDataFull> vulkanCommandQueuePool;
SystemDataPool<VulkanCommandListData, VulkanCommandListDataFull> vulkanCommandListPool;

thread_local CommandAllocatorDevicePool<VkCommandPool, VkCommandBuffer> threadVulkanDeviceCommandPools[VULKAN_MAX_DEVICES];

void InitVulkanCommandListMemory()
{
    if (!vulkanCommandQueuePool.Storage)
    {
        vulkanCommandQueuePool = SystemCreateDataPool<VulkanCommandQueueData, VulkanCommandQueueDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_COMMANDQUEUES);
        vulkanCommandListPool = SystemCreateDataPool<VulkanCommandListData, VulkanCommandListDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_COMMANDLISTS);
    }
}

VulkanCommandQueueData* GetVulkanCommandQueueData(ElemCommandQueue commandQueue)
{
    return SystemGetDataPoolItem(vulkanCommandQueuePool, commandQueue);
}

VulkanCommandQueueDataFull* GetVulkanCommandQueueDataFull(ElemCommandQueue commandQueue)
{
    return SystemGetDataPoolItemFull(vulkanCommandQueuePool, commandQueue);
}

VulkanCommandListData* GetVulkanCommandListData(ElemCommandList commandList)
{
    return SystemGetDataPoolItem(vulkanCommandListPool, commandList);
}

VulkanCommandListDataFull* GetVulkanCommandListDataFull(ElemCommandList commandList)
{
    return SystemGetDataPoolItemFull(vulkanCommandListPool, commandList);
}

ElemFence CreateVulkanCommandQueueFence(ElemCommandQueue commandQueue)
{
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetVulkanCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto fenceValue = SystemAtomicAdd(commandQueueData->FenceValue, 1) + 1;

    VkTimelineSemaphoreSubmitInfo timelineInfo = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    timelineInfo.waitSemaphoreValueCount = 0;
    timelineInfo.pWaitSemaphoreValues = nullptr;
    timelineInfo.signalSemaphoreValueCount = 1;
    timelineInfo.pSignalSemaphoreValues = &fenceValue;

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.pNext = &timelineInfo;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &commandQueueData->Fence;
    submitInfo.commandBufferCount = 0;
    submitInfo.pCommandBuffers = nullptr;
    submitInfo.pWaitDstStageMask = 0;

    AssertIfFailed(vkQueueSubmit(commandQueueData->DeviceObject, 1, &submitInfo, VK_NULL_HANDLE));

    return
    {
        .CommandQueue = commandQueue,
        .FenceValue = fenceValue
    };
}

ElemCommandQueue VulkanCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    InitVulkanCommandListMemory();
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    auto queueFamilyIndex = graphicsDeviceDataFull->RenderCommandQueueIndex;
    auto commandAllocatorQueueType = CommandAllocatorQueueType_Graphics;
    auto queueCurrentIndex = &graphicsDeviceDataFull->CurrentRenderCommandQueueIndex;

    if (type == ElemCommandQueueType_Compute)
    {
        queueFamilyIndex = graphicsDeviceDataFull->ComputeCommandQueueIndex;
        commandAllocatorQueueType = CommandAllocatorQueueType_Compute;
        queueCurrentIndex = &graphicsDeviceDataFull->CurrentComputeCommandQueueIndex;
    }

    // TODO: Check command Queue count

    VkQueue commandQueue;
    vkGetDeviceQueue(graphicsDeviceData->Device, queueFamilyIndex, *queueCurrentIndex++, &commandQueue);

    VkSemaphoreTypeCreateInfo timelineCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
    timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineCreateInfo.initialValue = 0;

    VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    createInfo.pNext = &timelineCreateInfo;

    VkSemaphore fence;
    AssertIfFailed(vkCreateSemaphore(graphicsDeviceData->Device, &createInfo, NULL, &fence));

    if (VulkanDebugLayerEnabled && options && options->DebugName)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_QUEUE;
        nameInfo.objectHandle = (uint64_t)commandQueue;
        nameInfo.pObjectName = options->DebugName;

        AssertIfFailed(vkSetDebugUtilsObjectNameEXT(graphicsDeviceData->Device, &nameInfo)); 
    } 

    auto commandAllocators = SystemPushArray<VkCommandPool>(VulkanGraphicsMemoryArena, VULKAN_MAX_COMMANDLISTS);
    auto commandLists = SystemPushArray<VkCommandBuffer>(VulkanGraphicsMemoryArena, VULKAN_MAX_COMMANDLISTS * VULKAN_MAX_COMMANDLISTS);

    auto handle = SystemAddDataPoolItem(vulkanCommandQueuePool, {
        .DeviceObject = commandQueue,
        .QueueFamilyIndex = queueFamilyIndex,
        .CommandAllocatorQueueType = commandAllocatorQueueType,
        .GraphicsDevice = graphicsDevice,
        .Fence = fence,
        .FenceValue = 0,
        .LastCompletedFenceValue = 0,
    }); 

    SystemAddDataPoolItemFull(vulkanCommandQueuePool, handle, {
        .CommandAllocators = commandAllocators,
        .CommandLists = commandLists
    });

    return handle;
}

void VulkanFreeCommandQueue(ElemCommandQueue commandQueue)
{ 
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetVulkanCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto commandQueueDataFull = GetVulkanCommandQueueDataFull(commandQueue);
    SystemAssert(commandQueueDataFull);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(commandQueueData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto fence = CreateVulkanCommandQueueFence(commandQueue);
    VulkanWaitForFenceOnCpu(fence);
    
    for (uint32_t i = 0; i < commandQueueDataFull->CurrentCommandAllocatorIndex; i++)
    {
        vkDestroyCommandPool(graphicsDeviceData->Device, commandQueueDataFull->CommandAllocators[i], nullptr);
    }

    for (uint32_t i = 0; i < commandQueueDataFull->CurrentCommandListIndex; i++)
    {
    }
    // TODO: Free allocators and command buffers

    vkDestroySemaphore(graphicsDeviceData->Device, commandQueueData->Fence, nullptr);
    
    SystemRemoveDataPoolItem(vulkanCommandQueuePool, commandQueue);
}

void VulkanResetCommandAllocation(ElemGraphicsDevice graphicsDevice)
{
    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    graphicsDeviceData->CommandAllocationGeneration++;
}

ElemCommandList VulkanGetCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options)
{
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetVulkanCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto commandQueueDataFull = GetVulkanCommandQueueDataFull(commandQueue);
    SystemAssert(commandQueueDataFull);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(commandQueueData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto commandAllocatorPoolItem = GetCommandAllocatorPoolItem(&threadVulkanDeviceCommandPools[commandQueueData->GraphicsDevice], 
                                                                graphicsDeviceData->CommandAllocationGeneration, 
                                                                commandQueueData->CommandAllocatorQueueType);

    if (commandAllocatorPoolItem->IsResetNeeded)
    {
        if (commandAllocatorPoolItem->CommandAllocator)
        {
            if (commandAllocatorPoolItem->Fence.FenceValue > 0)
            {
                VulkanWaitForFenceOnCpu(commandAllocatorPoolItem->Fence);
            }

            AssertIfFailed(vkResetCommandPool(graphicsDeviceData->Device, commandAllocatorPoolItem->CommandAllocator, 0));
        }
        else 
        {
            auto commandQueueDataFull = GetVulkanCommandQueueDataFull(commandQueue);
            SystemAssert(commandQueueDataFull);

            VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
            createInfo.queueFamilyIndex = commandQueueData->QueueFamilyIndex;

            VkCommandPool commandPool;
            AssertIfFailed(vkCreateCommandPool(graphicsDeviceData->Device, &createInfo, 0, &commandPool));

            commandAllocatorPoolItem->CommandAllocator = commandPool;

            auto commandAllocatorIndex = SystemAtomicAdd(commandQueueDataFull->CurrentCommandAllocatorIndex, 1);
            commandQueueDataFull->CommandAllocators[commandAllocatorIndex] = commandPool;

            for (uint32_t i = 0; i < MAX_COMMANDLIST; i++)
            {
                VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
                allocateInfo.commandPool = commandPool;
                allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocateInfo.commandBufferCount = 1;

                VkCommandBuffer commandBuffer;
                AssertIfFailed(vkAllocateCommandBuffers(graphicsDeviceData->Device, &allocateInfo, &commandBuffer));
                commandAllocatorPoolItem->CommandListPoolItems[i].CommandList = commandBuffer;

                auto commandListIndex = SystemAtomicAdd(commandQueueDataFull->CurrentCommandListIndex, 1);
                commandQueueDataFull->CommandLists[commandListIndex] = commandBuffer;
            }
        }

        commandAllocatorPoolItem->IsResetNeeded = false;
    }
    
    auto commandListPoolItem = GetCommandListPoolItem(commandAllocatorPoolItem);

    if (VulkanDebugLayerEnabled && options && options->DebugName)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
        nameInfo.objectHandle = (uint64_t)commandListPoolItem->CommandList;
        nameInfo.pObjectName = options->DebugName;

        AssertIfFailed(vkSetDebugUtilsObjectNameEXT(graphicsDeviceData->Device, &nameInfo)); 
    } 

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    AssertIfFailed(vkBeginCommandBuffer(commandListPoolItem->CommandList, &beginInfo));

    auto handle = SystemAddDataPoolItem(vulkanCommandListPool, {
        .DeviceObject = commandListPoolItem->CommandList,
        .GraphicsDevice = commandQueueData->GraphicsDevice,
        .CommandAllocatorPoolItem = commandAllocatorPoolItem,
        .CommandListPoolItem = commandListPoolItem
    }); 

    SystemAddDataPoolItemFull(vulkanCommandListPool, handle, {
    });

    return handle;
}

void VulkanCommitCommandList(ElemCommandList commandList)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    AssertIfFailed(vkEndCommandBuffer(commandListData->DeviceObject));
}

ElemFence VulkanExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    // TODO: Wait for fences if any

    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetVulkanCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);
/*
    VkPipelineStageFlags submitStageMasks[MAX_VULKAN_COMMAND_BUFFERS];
    VkSemaphore waitSemaphores[MAX_VULKAN_COMMAND_BUFFERS];
    uint64_t waitSemaphoreValues[MAX_VULKAN_COMMAND_BUFFERS];

    for (int32_t i = 0; i < fenceToWaitCount; i++)
    {
        auto fenceToWait = fencesToWait[i];
        auto commandQueueToWait = (VulkanCommandQueue*)fenceToWait.CommandQueuePointer;

        waitSemaphores[i] = commandQueueToWait->TimelineSemaphore;
        waitSemaphoreValues[i] = commandQueueToWait->FenceValue;
        submitStageMasks[i] = (commandQueue->CommandQueueType == CommandQueueType_Compute) ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    }
*/
    auto vulkanCommandBuffers = SystemPushArray<VkCommandBuffer>(stackMemoryArena, commandLists.Length);

    for (uint32_t i = 0; i < commandLists.Length; i++)
    {
        auto commandListData = GetVulkanCommandListData(commandLists.Items[i]);
        vulkanCommandBuffers[i] = commandListData->DeviceObject;
    }
    
    auto fenceValue = InterlockedIncrement(&commandQueueData->FenceValue);

    auto fence = ElemFence();
    fence.CommandQueue = commandQueue;
    fence.FenceValue = fenceValue;

    VkTimelineSemaphoreSubmitInfo timelineInfo = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    //timelineInfo.waitSemaphoreValueCount = (uint32_t)fenceToWaitCount;
    //timelineInfo.pWaitSemaphoreValues = (fenceToWaitCount > 0) ? waitSemaphoreValues : nullptr;
    timelineInfo.signalSemaphoreValueCount = 1;
    timelineInfo.pSignalSemaphoreValues = &fenceValue;

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    //submitInfo.waitSemaphoreCount = (uint32_t)fenceToWaitCount;
    //submitInfo.pWaitSemaphores = (fenceToWaitCount > 0) ? waitSemaphores : nullptr;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &commandQueueData->Fence;
    submitInfo.commandBufferCount = (uint32_t)commandLists.Length;
    submitInfo.pCommandBuffers = vulkanCommandBuffers.Pointer;
    //submitInfo.pWaitDstStageMask = submitStageMasks;
    submitInfo.pNext = &timelineInfo;

    AssertIfFailed(vkQueueSubmit(commandQueueData->DeviceObject, 1, &submitInfo, VK_NULL_HANDLE));

    for (uint32_t i = 0; i < commandLists.Length; i++)
    {
        auto commandListData = GetVulkanCommandListData(commandLists.Items[i]);
        UpdateCommandAllocatorPoolItemFence(commandListData->CommandAllocatorPoolItem, fence);
        ReleaseCommandListPoolItem(commandListData->CommandListPoolItem);

        // TODO: Release VkCommandBuffer?
        SystemRemoveDataPoolItem(vulkanCommandListPool, commandLists.Items[i]);
    }

    return fence;
}

void VulkanWaitForFenceOnCpu(ElemFence fence)
{
    SystemAssert(fence.CommandQueue != ELEM_HANDLE_NULL);

    auto commandQueueToWaitData = GetVulkanCommandQueueData(fence.CommandQueue);
    SystemAssert(commandQueueToWaitData);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(commandQueueToWaitData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    if (fence.FenceValue > commandQueueToWaitData->LastCompletedFenceValue) 
    {
        uint64_t semaphoreValue;
        vkGetSemaphoreCounterValue(graphicsDeviceData->Device, commandQueueToWaitData->Fence, &semaphoreValue);

        commandQueueToWaitData->LastCompletedFenceValue = max(commandQueueToWaitData->LastCompletedFenceValue, semaphoreValue);
    }

    if (fence.FenceValue > commandQueueToWaitData->LastCompletedFenceValue)
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Wait for fence on CPU...");

        VkSemaphoreWaitInfo waitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &commandQueueToWaitData->Fence;
        waitInfo.pValues = &fence.FenceValue;

        AssertIfFailed(vkWaitSemaphores(graphicsDeviceData->Device, &waitInfo, UINT64_MAX));
    }
}
