#pragma once

void MetalBeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassParameters* parameters);
void MetalEndRenderPass(ElemCommandList commandList);
void MetalDispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
