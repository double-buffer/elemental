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
void VulkanSetSwapChainTiming(ElemSwapChain swapChain, uint32_t frameLatency, uint32_t targetFPS);
void VulkanPresentSwapChain(ElemSwapChain swapChain);
