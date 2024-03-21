#include "VulkanGraphicsDevice.h"
#include "VulkanCommandList.h"
#include "SystemLogging.h"

void VulkanBeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassOptions* options)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Begin render");
}

void VulkanEndRenderPass(ElemCommandList commandList)
{
}
