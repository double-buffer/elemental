#pragma once

#include "Elemental.h"

void VulkanDispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
