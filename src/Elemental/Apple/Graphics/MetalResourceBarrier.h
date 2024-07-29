#pragma once

#include "Elemental.h"
#include "Graphics/ResourceBarrier.h"

void InsertMetalResourceBarriersIfNeeded(ElemCommandList commandList, ElemGraphicsResourceBarrierSyncType currentStage);
void MetalGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options);
