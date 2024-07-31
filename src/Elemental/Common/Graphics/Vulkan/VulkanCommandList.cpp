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
thread_local bool threadVulkanCommandBufferCommitted = true;

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

    createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VkSemaphore presentSemaphore;
    AssertIfFailed(vkCreateSemaphore(graphicsDeviceData->Device, &createInfo, NULL, &presentSemaphore));

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
        .PresentSemaphore = presentSemaphore,
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

    vkDestroySemaphore(graphicsDeviceData->Device, commandQueueData->PresentSemaphore, nullptr);
    vkDestroySemaphore(graphicsDeviceData->Device, commandQueueData->Fence, nullptr);
    
    auto graphicsIdUnpacked = UnpackSystemDataPoolHandle(commandQueueData->GraphicsDevice);
    threadVulkanDeviceCommandPools[graphicsIdUnpacked.Index] = {};

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
    if (!threadVulkanCommandBufferCommitted)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Cannot get a command list if commit was not called on the same thread.");
        return ELEM_HANDLE_NULL;
    }

    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetVulkanCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto commandQueueDataFull = GetVulkanCommandQueueDataFull(commandQueue);
    SystemAssert(commandQueueDataFull);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(commandQueueData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsIdUnpacked = UnpackSystemDataPoolHandle(commandQueueData->GraphicsDevice);
    auto commandAllocatorPoolItem = GetCommandAllocatorPoolItem(&threadVulkanDeviceCommandPools[graphicsIdUnpacked.Index], 
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
    
    auto resourceBarrierPool = CreateResourceBarrierPool(VulkanGraphicsMemoryArena);

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    AssertIfFailed(vkBeginCommandBuffer(commandListPoolItem->CommandList, &beginInfo));

    auto handle = SystemAddDataPoolItem(vulkanCommandListPool, {
        .DeviceObject = commandListPoolItem->CommandList,
        .GraphicsDevice = commandQueueData->GraphicsDevice,
        .CommandQueue = commandQueue,
        .CommandAllocatorPoolItem = commandAllocatorPoolItem,
        .CommandListPoolItem = commandListPoolItem,
        .ResourceBarrierPool = resourceBarrierPool
    }); 

    SystemAddDataPoolItemFull(vulkanCommandListPool, handle, {
    });
    
    threadVulkanCommandBufferCommitted = false;

    return handle;
}

void VulkanCommitCommandList(ElemCommandList commandList)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    AssertIfFailed(vkEndCommandBuffer(commandListData->DeviceObject));
    
    commandListData->IsCommitted = true;
    threadVulkanCommandBufferCommitted = true;
}

ElemFence VulkanExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetVulkanCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    Span<VkPipelineStageFlags> submitStageMasks = {};
    Span<VkSemaphore> waitSemaphores = {};
    Span<uint64_t> waitSemaphoreValues = {};

    if (options && options->FencesToWait.Length > 0)
    {
        submitStageMasks = SystemPushArray<VkPipelineStageFlags>(stackMemoryArena, options->FencesToWait.Length);
        waitSemaphores = SystemPushArray<VkSemaphore>(stackMemoryArena, options->FencesToWait.Length);
        waitSemaphoreValues = SystemPushArray<uint64_t>(stackMemoryArena, options->FencesToWait.Length);

        for (uint32_t i = 0; i < options->FencesToWait.Length; i++)
        {
            auto fenceToWait = options->FencesToWait.Items[i];

            auto commandQueueToWaitData = GetVulkanCommandQueueData(fenceToWait.CommandQueue);
            SystemAssert(commandQueueToWaitData);

            submitStageMasks[i] = VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT | VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            waitSemaphores[i] = commandQueueToWaitData->Fence;
            waitSemaphoreValues[i] = fenceToWait.FenceValue;
    
            if (VulkanDebugBarrierInfoEnabled)
            {
                SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Waiting for fence before ExecuteCommandLists. (CommandQueue=%d, Value=%d)", fenceToWait.CommandQueue, fenceToWait.FenceValue);
            }
        }
    }

    bool hasError = false;
    auto vulkanCommandBuffers = SystemPushArray<VkCommandBuffer>(stackMemoryArena, commandLists.Length);

    for (uint32_t i = 0; i < commandLists.Length; i++)
    {
        auto commandListData = GetVulkanCommandListData(commandLists.Items[i]);

        if (!commandListData->IsCommitted)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Commandlist needs to be committed before executing it.");
            hasError = true;
            break;
        }

        vulkanCommandBuffers[i] = commandListData->DeviceObject;
    }
    
    auto fenceValue = SystemAtomicAdd(commandQueueData->FenceValue, 1) + 1;

    if (!hasError)
    {
        uint32_t signalCount = 1u;

        if (commandQueueData->SignalPresentSemaphore)
        {
            signalCount = 2u;
            commandQueueData->SignalPresentSemaphore = false;
        }

        uint64_t signalValues[] = { fenceValue, 0u };

        VkTimelineSemaphoreSubmitInfo timelineInfo = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
        timelineInfo.waitSemaphoreValueCount = waitSemaphoreValues.Length;
        timelineInfo.pWaitSemaphoreValues = waitSemaphoreValues.Pointer;
        timelineInfo.signalSemaphoreValueCount = signalCount;
        timelineInfo.pSignalSemaphoreValues = signalValues;

        VkSemaphore signalSemaphores[] = { commandQueueData->Fence, commandQueueData->PresentSemaphore };

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = waitSemaphores.Length;
        submitInfo.pWaitSemaphores = waitSemaphores.Pointer;
        submitInfo.signalSemaphoreCount = signalCount;
        submitInfo.pSignalSemaphores = signalSemaphores;
        submitInfo.commandBufferCount = (uint32_t)commandLists.Length;
        submitInfo.pCommandBuffers = vulkanCommandBuffers.Pointer;
        submitInfo.pWaitDstStageMask = submitStageMasks.Pointer;
        submitInfo.pNext = &timelineInfo;

        AssertIfFailed(vkQueueSubmit(commandQueueData->DeviceObject, 1, &submitInfo, VK_NULL_HANDLE));
    }

    auto fence = ElemFence();
    fence.CommandQueue = commandQueue;
    fence.FenceValue = fenceValue;

    for (uint32_t i = 0; i < commandLists.Length; i++)
    {
        auto commandListData = GetVulkanCommandListData(commandLists.Items[i]);
        
        if (!commandListData->IsCommitted)
        {
            VulkanCommitCommandList(commandLists.Items[i]);
        }

        UpdateCommandAllocatorPoolItemFence(commandListData->CommandAllocatorPoolItem, fence);
        ReleaseCommandListPoolItem(commandListData->CommandListPoolItem);
        FreeResourceBarrierPool(commandListData->ResourceBarrierPool);

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

        commandQueueToWaitData->LastCompletedFenceValue = SystemMax(commandQueueToWaitData->LastCompletedFenceValue, semaphoreValue);
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

bool VulkanIsFenceCompleted(ElemFence fence)
{
    SystemAssert(fence.CommandQueue != ELEM_HANDLE_NULL);

    auto commandQueueToWaitData = GetVulkanCommandQueueData(fence.CommandQueue);
    auto commandQueueToWaitDataFull = GetVulkanCommandQueueDataFull(fence.CommandQueue);

    if (!commandQueueToWaitData)
    {
        return true;
    }

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(commandQueueToWaitData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    if (fence.FenceValue > commandQueueToWaitData->LastCompletedFenceValue) 
    {
        uint64_t semaphoreValue;
        vkGetSemaphoreCounterValue(graphicsDeviceData->Device, commandQueueToWaitData->Fence, &semaphoreValue);

        commandQueueToWaitData->LastCompletedFenceValue = SystemMax(commandQueueToWaitData->LastCompletedFenceValue, semaphoreValue);
    }

    return fence.FenceValue <= commandQueueToWaitData->LastCompletedFenceValue;
}
