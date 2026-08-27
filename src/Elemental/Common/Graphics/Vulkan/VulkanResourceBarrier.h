#pragma once

#include "Elemental.h"
#include "Graphics/ResourceBarrier.h"

void InsertVulkanResourceBarriersIfNeeded(ElemCommandList commandList, ElemGraphicsResourceBarrierSyncType currentStage);

void VulkanGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options);
void VulkanGraphicsResourceBarrierResource(ElemCommandList commandList, ElemGraphicsResource resource, ElemGraphicsResourceBarrierAccessType accessType);
