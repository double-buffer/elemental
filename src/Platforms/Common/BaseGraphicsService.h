#pragma once
#include "Elemental.h"

class BaseGraphicsService
{
public:
    virtual ~BaseGraphicsService() {};

    virtual void GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count) = 0;
    virtual void* CreateGraphicsDevice(GraphicsDeviceOptions* options) = 0;
    virtual void FreeGraphicsDevice(void* graphicsDevicePointer) = 0;
    virtual GraphicsDeviceInfo GetGraphicsDeviceInfo(void* graphicsDevicePointer) = 0;
    
    virtual void* CreateCommandQueue(void* graphicsDevicePointer, CommandQueueType type) = 0;
    virtual void FreeCommandQueue(void* commandQueuePointer) = 0;
    virtual void SetCommandQueueLabel(void* commandQueuePointer, uint8_t* label) = 0;

    virtual void* CreateCommandList(void* commandQueuePointer) = 0;
    virtual void FreeCommandList(void* commandListPointer) = 0;
    virtual void SetCommandListLabel(void* commandListPointer, uint8_t* label) = 0;    
    virtual void CommitCommandList(void* commandList) = 0;

    virtual Fence ExecuteCommandLists(void* commandQueuePointer, void** commandLists, int32_t commandListCount, Fence* fencesToWait, int32_t fenceToWaitCount) = 0;
    virtual void WaitForFenceOnCpu(Fence fence) = 0;
    virtual void ResetCommandAllocation(void* graphicsDevice) = 0;

    virtual void FreeTexture(void* texturePointer) = 0;

    virtual void* CreateSwapChain(void* windowPointer, void* commandQueuePointer, SwapChainOptions* options) = 0;
    virtual void FreeSwapChain(void* swapChainPointer) = 0;
    virtual void ResizeSwapChain(void* swapChainPointer, int width, int height) = 0;
    virtual void* GetSwapChainBackBufferTexture(void* swapChainPointer) = 0;
    virtual void PresentSwapChain(void* swapChainPointer) = 0;
    virtual void WaitForSwapChainOnCpu(void* swapChainPointer) = 0;

    virtual void* CreateShader(void* graphicsDevicePointer, ShaderPart* shaderParts, int32_t shaderPartCount) = 0;
    virtual void FreeShader(void* shaderPointer) = 0;

    virtual void BeginRenderPass(void* commandListPointer, RenderPassDescriptor* renderPassDescriptor) = 0;
    virtual void EndRenderPass(void* commandListPointer) = 0;

    virtual void SetShader(void* commandListPointer, void* shaderPointer) = 0;
    virtual void SetShaderConstants(void* commandListPointer, uint32_t slot, void* constantValues, int32_t constantValueCount) = 0;

    virtual void DispatchMesh(void* commandListPointer, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;
};