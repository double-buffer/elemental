#include "VulkanSwapChain.h"
#include "VulkanGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define VULKAN_MAX_SWAPCHAINS 10u

SystemDataPool<VulkanSwapChainData, VulkanSwapChainDataFull> vulkanSwapChainPool;

void InitVulkanSwapChainMemory()
{
    if (!vulkanSwapChainPool.Storage)
    {
        vulkanSwapChainPool = SystemCreateDataPool<VulkanSwapChainData, VulkanSwapChainDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_SWAPCHAINS);
    }
}

VulkanSwapChainData* GetVulkanSwapChainData(ElemSwapChain swapChain)
{
    return SystemGetDataPoolItem(vulkanSwapChainPool, swapChain);
}

VulkanSwapChainDataFull* GetVulkanSwapChainDataFull(ElemSwapChain swapChain)
{
    return SystemGetDataPoolItemFull(vulkanSwapChainPool, swapChain);
}

ElemSwapChain VulkanCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options)
{
    return ELEM_HANDLE_NULL;
}

void VulkanFreeSwapChain(ElemSwapChain swapChain)
{
}

ElemSwapChainInfo VulkanGetSwapChainInfo(ElemSwapChain swapChain)
{
    return {};
}

void VulkanResizeSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height)
{
}

ElemTexture VulkanGetSwapChainBackBufferTexture(ElemSwapChain swapChain)
{
    return ELEM_HANDLE_NULL;
}

void VulkanPresentSwapChain(ElemSwapChain swapChain)
{
}

void VulkanWaitForSwapChainOnCpu(ElemSwapChain swapChain)
{
}
