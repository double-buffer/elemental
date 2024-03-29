#include "VulkanRendering.h"
#include "SystemLogging.h"

void VulkanBeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassParameters* parameters)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Begin render");
}

void VulkanEndRenderPass(ElemCommandList commandList)
{
}

void VulkanDispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
}
