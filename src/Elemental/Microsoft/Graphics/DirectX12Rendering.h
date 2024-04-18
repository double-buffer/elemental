#pragma once

#include "Elemental.h"

void DirectX12BeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassParameters* parameters);
void DirectX12EndRenderPass(ElemCommandList commandList);
void DirectX12SetViewports(ElemCommandList commandList, ElemViewportSpan viewports);
void DirectX12DispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
