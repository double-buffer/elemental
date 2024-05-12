#pragma once

#include "Elemental.h"
#include "volk.h"

void VulkanSetViewports(ElemCommandList commandList, ElemViewportSpan viewports);
void VulkanDispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
