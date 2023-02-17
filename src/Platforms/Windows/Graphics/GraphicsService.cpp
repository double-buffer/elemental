#include "WindowsCommon.h"
#include "Direct3D12/Direct3D12GraphicsService.h"
#include "../../Common/BaseGraphicsObject.h"
#include "../../Common/BaseGraphicsService.h"
#include "../../Common/Vulkan/VulkanGraphicsService.h"

static Direct3D12GraphicsService* _globalDirect3D12GraphicsService; 
static VulkanGraphicsService* _globalVulkanGraphicsService; 

DllExport void Native_InitGraphicsService(GraphicsServiceOptions options)
{
    _globalDirect3D12GraphicsService = new Direct3D12GraphicsService(options);
    _globalVulkanGraphicsService = new VulkanGraphicsService(options);
}

DllExport void Native_FreeGraphicsService()
{
    delete _globalDirect3D12GraphicsService;
    delete _globalVulkanGraphicsService;
}

DllExport void Native_GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count)
{
    (*count) = 0;

    _globalDirect3D12GraphicsService->GetAvailableGraphicsDevices(graphicsDevices, count);
    _globalVulkanGraphicsService->GetAvailableGraphicsDevices(graphicsDevices, count);
}

DllExport void* Native_CreateGraphicsDevice(GraphicsDeviceOptions options)
{
    GraphicsDeviceInfo availableDevices[50];
    int availableDeviceCount;

    Native_GetAvailableGraphicsDevices(availableDevices, &availableDeviceCount);

    if (availableDeviceCount == 0)
    {
        return nullptr;
    }

    auto selectedDevice = availableDevices[0];

    if (options.DeviceId != 0)
    {
        for (int i = 0; i < availableDeviceCount; i++)
        {
            if (availableDevices[i].DeviceId == options.DeviceId)
            {
                selectedDevice = availableDevices[i];
                break;
            }
        }
    }

    options.DeviceId = selectedDevice.DeviceId;

    BaseGraphicsService* graphicsService;

    if (selectedDevice.GraphicsApi == GraphicsApi_Vulkan)
    {
        graphicsService = (BaseGraphicsService*)_globalVulkanGraphicsService;
    }
    else
    {
        graphicsService = (BaseGraphicsService*)_globalDirect3D12GraphicsService;
    }

    return graphicsService->CreateGraphicsDevice(options);
}

DllExport void Native_FreeGraphicsDevice(void* graphicsDevicePointer)
{
    auto graphicsService = ((BaseGraphicsObject*)graphicsDevicePointer)->GraphicsService;
    graphicsService->FreeGraphicsDevice(graphicsDevicePointer);
}

DllExport GraphicsDeviceInfo Native_GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    auto graphicsService = ((BaseGraphicsObject*)graphicsDevicePointer)->GraphicsService;
    return graphicsService->GetGraphicsDeviceInfo(graphicsDevicePointer);
}

DllExport void* Native_CreateCommandQueue(void* graphicsDevicePointer, CommandQueueType type)
{
    auto graphicsService = ((BaseGraphicsObject*)graphicsDevicePointer)->GraphicsService;
    return graphicsService->CreateCommandQueue(graphicsDevicePointer, type);
}

DllExport void Native_FreeCommandQueue(void* commandQueuePointer)
{
    auto graphicsService = ((BaseGraphicsObject*)commandQueuePointer)->GraphicsService;
    graphicsService->FreeCommandQueue(commandQueuePointer);
}

DllExport void Native_SetCommandQueueLabel(void* commandQueuePointer, uint8_t* label)
{
    auto graphicsService = ((BaseGraphicsObject*)commandQueuePointer)->GraphicsService;
    graphicsService->SetCommandQueueLabel(commandQueuePointer, label);
}

DllExport void* Native_CreateCommandList(void* commandQueuePointer)
{
    auto graphicsService = ((BaseGraphicsObject*)commandQueuePointer)->GraphicsService;
    return graphicsService->CreateCommandList(commandQueuePointer);
}

DllExport void Native_FreeCommandList(void* commandListPointer)
{
    auto graphicsService = ((BaseGraphicsObject*)commandListPointer)->GraphicsService;
    graphicsService->FreeCommandList(commandListPointer);
}

DllExport void Native_SetCommandListLabel(void* commandListPointer, uint8_t* label)
{
    auto graphicsService = ((BaseGraphicsObject*)commandListPointer)->GraphicsService;
    graphicsService->SetCommandListLabel(commandListPointer, label);
}

DllExport void Native_CommitCommandList(void* commandListPointer)
{
    auto graphicsService = ((BaseGraphicsObject*)commandListPointer)->GraphicsService;
    graphicsService->CommitCommandList(commandListPointer);
}
    
DllExport Fence Native_ExecuteCommandLists(void* commandQueuePointer, void** commandLists, int32_t commandListCount, Fence* fencesToWait, int32_t fenceToWaitCount)
{
    auto graphicsService = ((BaseGraphicsObject*)commandQueuePointer)->GraphicsService;
    return graphicsService->ExecuteCommandLists(commandQueuePointer, commandLists, commandListCount, fencesToWait, fenceToWaitCount);
}
    
DllExport void Native_WaitForFenceOnCpu(Fence fence)
{
    auto graphicsService = ((BaseGraphicsObject*)fence.CommandQueuePointer)->GraphicsService;
    graphicsService->WaitForFenceOnCpu(fence);
}

DllExport void* Native_CreateSwapChain(void* windowPointer, void* commandQueuePointer, SwapChainOptions options)
{
    auto graphicsService = ((BaseGraphicsObject*)commandQueuePointer)->GraphicsService;
    return graphicsService->CreateSwapChain(windowPointer, commandQueuePointer, options);
}

DllExport void Native_FreeSwapChain(void* swapChainPointer)
{
    auto graphicsService = ((BaseGraphicsObject*)swapChainPointer)->GraphicsService;
    graphicsService->FreeSwapChain(swapChainPointer);
}

DllExport void* Native_GetSwapChainBackBufferTexture(void* swapChainPointer)
{
    auto graphicsService = ((BaseGraphicsObject*)swapChainPointer)->GraphicsService;
    return graphicsService->GetSwapChainBackBufferTexture(swapChainPointer);
}

DllExport void Native_PresentSwapChain(void* swapChainPointer)
{
    auto graphicsService = ((BaseGraphicsObject*)swapChainPointer)->GraphicsService;
    graphicsService->PresentSwapChain(swapChainPointer);
}

DllExport void Native_WaitForSwapChainOnCpu(void* swapChainPointer)
{
    auto graphicsService = ((BaseGraphicsObject*)swapChainPointer)->GraphicsService;
    graphicsService->WaitForSwapChainOnCpu(swapChainPointer);
}

DllExport void Native_BeginRenderPass(void* commandListPointer, RenderPassDescriptor* renderPassDescriptor)
{
    auto graphicsService = ((BaseGraphicsObject*)commandListPointer)->GraphicsService;
    graphicsService->BeginRenderPass(commandListPointer, renderPassDescriptor);
}
    
DllExport void Native_EndRenderPass(void* commandListPointer)
{
    auto graphicsService = ((BaseGraphicsObject*)commandListPointer)->GraphicsService;
    graphicsService->EndRenderPass(commandListPointer);
}