#pragma once

#include "Elemental.h"

struct VulkanSwapChainData
{
};

struct VulkanSwapChainDataFull
{
};

VulkanSwapChainData* GetVulkanSwapChainData(ElemSwapChain swapChain);
VulkanSwapChainDataFull* GetVulkanSwapChainDataFull(ElemSwapChain swapChain);

ElemSwapChain VulkanCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, const ElemSwapChainOptions* options);
void VulkanFreeSwapChain(ElemSwapChain swapChain);
void VulkanResizeSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height);
void VulkanPresentSwapChain(ElemSwapChain swapChain);
void VulkanWaitForSwapChainOnCpu(ElemSwapChain swapChain);
