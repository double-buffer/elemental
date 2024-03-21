#include "DirectX12GraphicsDevice.h"
#include "DirectX12CommandList.h"
#include "SystemLogging.h"
#include "SystemFunctions.h"

void DirectX12BeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassOptions* options)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    if (options && options->RenderTargets.Length > 0)
    {
        SystemAssert(options->RenderTargets.Items[0].RenderTarget != ELEM_HANDLE_NULL);
    }
}

void DirectX12EndRenderPass(ElemCommandList commandList)
{
}
