#include "BaseGraphicsObject.h"
#include "BaseGraphicsService.h"

#define DispatchReturnGraphicsFunction(functionName, apiObject, ...) GraphicsObject* graphicsObject = (GraphicsObject*)apiObject; \
    switch (graphicsObject->GraphicsApi) { \
        case GraphicsApi_Direct3D12: return Direct3D12##functionName(apiObject, __VA_ARGS__); \
        case GraphicsApi_Vulkan: \
        case GraphicsApi_Metal: \
        case GraphicsApi_Unknown: \
        default: printf("Error: Unknown graphics API.\n"); return {}; \
    }

#define DispatchGraphicsFunction(functionName, apiObject, ...) GraphicsObject* graphicsObject = (GraphicsObject*)apiObject; \
    switch (graphicsObject->GraphicsApi) { \
        case GraphicsApi_Direct3D12: Direct3D12##functionName(apiObject, __VA_ARGS__); break; \
        case GraphicsApi_Vulkan: \
        case GraphicsApi_Metal: \
        case GraphicsApi_Unknown: \
        default: printf("Error: Unknown graphics API.\n");\
    }

#define DispatchGraphicsFunctionForApi(functionName, api, ...) \
    switch (api) { \
        case GraphicsApi_Direct3D12: Direct3D12##functionName(__VA_ARGS__); break; \
        case GraphicsApi_Vulkan: \
        case GraphicsApi_Metal: \
        case GraphicsApi_Unknown: \
        default: printf("Error: Unknown graphics API.\n");\
    }

static VulkanGraphicsService* _globalVulkanGraphicsService; 

DllExport void Native_InitGraphicsService(GraphicsServiceOptions* options)
{
   Direct3D12InitGraphicsService(options);
    _globalVulkanGraphicsService = new VulkanGraphicsService(options);
}

DllExport void Native_FreeGraphicsService()
{
    Direct3D12FreeGraphicsService();
    delete _globalVulkanGraphicsService;
}

DllExport void Native_GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count)
{
    (*count) = 0;

    Direct3D12GetAvailableGraphicsDevices(graphicsDevices, count);
    _globalVulkanGraphicsService->GetAvailableGraphicsDevices(graphicsDevices, count);
}

DllExport void* Native_CreateGraphicsDevice(GraphicsDeviceOptions* options)
{
    GraphicsDeviceInfo availableDevices[50];
    int availableDeviceCount;

    Native_GetAvailableGraphicsDevices(availableDevices, &availableDeviceCount);

    if (availableDeviceCount == 0)
    {
        return nullptr;
    }

    auto selectedDevice = availableDevices[0];

    if (options->DeviceId != 0)
    {
        for (int i = 0; i < availableDeviceCount; i++)
        {
            if (availableDevices[i].DeviceId == options->DeviceId)
            {
                selectedDevice = availableDevices[i];
                break;
            }
        }
    }
    
    for (int i = 0; i < availableDeviceCount; i++)
    {
        delete availableDevices[i].DeviceName;
    }

    options->DeviceId = selectedDevice.DeviceId;

    BaseGraphicsService* graphicsService;

    if (selectedDevice.GraphicsApi == GraphicsApi_Vulkan)
    {
        graphicsService = (BaseGraphicsService*)_globalVulkanGraphicsService;
    }
    else
    {
        return Direct3D12CreateGraphicsDevice(options);
    }

    return graphicsService->CreateGraphicsDevice(options);
}

DllExport void Native_FreeGraphicsDevice(void* graphicsDevicePointer)
{
    DispatchGraphicsFunction(FreeGraphicsDevice, graphicsDevicePointer);
}

DllExport GraphicsDeviceInfo Native_GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    DispatchReturnGraphicsFunction(GetGraphicsDeviceInfo, graphicsDevicePointer);
}

DllExport void* Native_CreateCommandQueue(void* graphicsDevicePointer, CommandQueueType type)
{
    DispatchReturnGraphicsFunction(CreateCommandQueue, graphicsDevicePointer, type);
}

DllExport void Native_FreeCommandQueue(void* commandQueuePointer)
{
    DispatchGraphicsFunction(FreeCommandQueue, commandQueuePointer);
}

DllExport void Native_SetCommandQueueLabel(void* commandQueuePointer, uint8_t* label)
{
    DispatchGraphicsFunction(SetCommandQueueLabel, commandQueuePointer, label);
}

DllExport void* Native_CreateCommandList(void* commandQueuePointer)
{
    DispatchReturnGraphicsFunction(CreateCommandList, commandQueuePointer);
}

DllExport void Native_FreeCommandList(void* commandListPointer)
{
    DispatchGraphicsFunction(FreeCommandList, commandListPointer);
}

DllExport void Native_SetCommandListLabel(void* commandListPointer, uint8_t* label)
{
    DispatchGraphicsFunction(SetCommandListLabel, commandListPointer, label);
}

DllExport void Native_CommitCommandList(void* commandListPointer)
{
    DispatchGraphicsFunction(CommitCommandList, commandListPointer);
}
    
DllExport Fence Native_ExecuteCommandLists(void* commandQueuePointer, void** commandLists, int32_t commandListCount, Fence* fencesToWait, int32_t fenceToWaitCount)
{
    DispatchReturnGraphicsFunction(ExecuteCommandLists, commandQueuePointer, commandLists, commandListCount, fencesToWait, fenceToWaitCount);
}
    
DllExport void Native_WaitForFenceOnCpu(Fence fence)
{
    DispatchGraphicsFunctionForApi(WaitForFenceOnCpu, ((GraphicsObject*)fence.CommandQueuePointer)->GraphicsApi, fence);
}
    
DllExport void Native_ResetCommandAllocation(void* graphicsDevicePointer)
{
    DispatchGraphicsFunction(ResetCommandAllocation, graphicsDevicePointer);
}
    
DllExport void Native_FreeTexture(void* texturePointer)
{
    DispatchGraphicsFunction(FreeTexture, texturePointer);
}

DllExport void* Native_CreateSwapChain(void* windowPointer, void* commandQueuePointer, SwapChainOptions* options)
{
    DispatchReturnGraphicsFunction(CreateSwapChain, commandQueuePointer, windowPointer, options);
}

DllExport void Native_FreeSwapChain(void* swapChainPointer)
{
    DispatchGraphicsFunction(FreeSwapChain, swapChainPointer);
}
    
DllExport void Native_ResizeSwapChain(void* swapChainPointer, int width, int height)
{
    DispatchGraphicsFunction(ResizeSwapChain, swapChainPointer, width, height);
}

DllExport void* Native_GetSwapChainBackBufferTexture(void* swapChainPointer)
{
    DispatchReturnGraphicsFunction(GetSwapChainBackBufferTexture, swapChainPointer);
}

DllExport void Native_PresentSwapChain(void* swapChainPointer)
{
    DispatchGraphicsFunction(PresentSwapChain, swapChainPointer);
}

DllExport void Native_WaitForSwapChainOnCpu(void* swapChainPointer)
{
    DispatchGraphicsFunction(WaitForSwapChainOnCpu, swapChainPointer);
}
    
DllExport void* Native_CreateShader(void* graphicsDevicePointer, ShaderPart* shaderParts, int32_t shaderPartCount)
{
    DispatchReturnGraphicsFunction(CreateShader, graphicsDevicePointer, shaderParts, shaderPartCount);
}

DllExport void Native_FreeShader(void* shaderPointer)
{
    DispatchGraphicsFunction(FreeShader, shaderPointer);
}

DllExport void Native_BeginRenderPass(void* commandListPointer, RenderPassDescriptor* renderPassDescriptor)
{
    DispatchGraphicsFunction(BeginRenderPass, commandListPointer, renderPassDescriptor);
}
    
DllExport void Native_EndRenderPass(void* commandListPointer)
{
    DispatchGraphicsFunction(EndRenderPass, commandListPointer);
}

DllExport void Native_SetShader(void* commandListPointer, void* shaderPointer)
{
    DispatchGraphicsFunction(SetShader, commandListPointer, shaderPointer);
}
    
DllExport void Native_SetShaderConstants(void* commandListPointer, uint32_t slot, void* constantValues, int32_t constantValueCount)
{
    DispatchGraphicsFunction(SetShaderConstants, commandListPointer, slot, constantValues, constantValueCount);
}
    
DllExport void Native_DispatchMesh(void* commandListPointer, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    DispatchGraphicsFunction(DispatchMesh, commandListPointer, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}