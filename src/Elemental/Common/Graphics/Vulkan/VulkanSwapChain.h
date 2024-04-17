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

ElemSwapChain VulkanCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options);
void VulkanFreeSwapChain(ElemSwapChain swapChain);
ElemSwapChainInfo VulkanGetSwapChainInfo(ElemSwapChain swapChain);
void VulkanPresentSwapChain(ElemSwapChain swapChain);
