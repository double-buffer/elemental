#include "MetalGraphicsDevice.h"
#include "MetalCommandList.h"
#include "SystemLogging.h"

void MetalBeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassOptions* options)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Begin render");
}

void MetalEndRenderPass(ElemCommandList commandList)
{
}
