#pragma once
#include "BaseGraphicsService.h"
#include "BaseGraphicsObject.h"

#include "MacOSWindow.h"

#include "MetalGraphicsDevice.h"
#include "MetalBaseGraphicsObject.h"
#include "MetalCommandQueue.h"
#include "MetalCommandList.h"
#include "MetalTexture.h"
#include "MetalSwapChain.h"
#include "MetalShader.h"

class MetalGraphicsService : BaseGraphicsService
{
public:
    MetalGraphicsService(GraphicsServiceOptions* options);
    ~MetalGraphicsService();

    void GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count) override;
    void* CreateGraphicsDevice(GraphicsDeviceOptions* options) override;
    void FreeGraphicsDevice(void* graphicsDevicePointer) override;
    GraphicsDeviceInfo GetGraphicsDeviceInfo(void* graphicsDevicePointer) override;
    
    void* CreateCommandQueue(void* graphicsDevicePointer, CommandQueueType type) override;
    void FreeCommandQueue(void* commandQueuePointer) override;
    void SetCommandQueueLabel(void* commandQueuePointer, uint8_t* label) override;
    
    void* CreateCommandList(void* commandQueuePointer) override;
    void FreeCommandList(void* commandListPointer) override;
    void SetCommandListLabel(void* commandListPointer, uint8_t* label) override;
    void CommitCommandList(void* commandList) override;
    
    Fence ExecuteCommandLists(void* commandQueuePointer, void** commandLists, int32_t commandListCount, Fence* fencesToWait, int32_t fenceToWaitCount) override;
    void WaitForFenceOnCpu(Fence fence) override;
    void ResetCommandAllocation(void* graphicsDevicePointer) override;
    
    void FreeTexture(void* texturePointer) override;
    
    void* CreateSwapChain(void* windowPointer, void* commandQueuePointer, SwapChainOptions* options) override;
    void FreeSwapChain(void* swapChainPointer) override;
    void ResizeSwapChain(void* swapChainPointer, int width, int height) override;
    void* GetSwapChainBackBufferTexture(void* swapChainPointer) override;
    void PresentSwapChain(void* swapChainPointer) override;
    void WaitForSwapChainOnCpu(void* swapChainPointer) override;

    void* CreateShader(void* graphicsDevicePointer, ShaderPart* shaderParts, int32_t shaderPartCount) override;
    void FreeShader(void* shaderPointer) override;
    
    void BeginRenderPass(void* commandListPointer, RenderPassDescriptor* renderPassDescriptor) override;
    void EndRenderPass(void* commandListPointer) override;
    
    void SetShader(void* commandListPointer, void* shaderPointer) override;
    void SetShaderConstants(void* commandListPointer, uint32_t slot, void* constantValues, int32_t constantValueCount) override;

    void DispatchMesh(void* commandListPointer, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) override;

private:
    NS::SharedPtr<MTL::SharedEventListener> _sharedEventListener;
    
    GraphicsDeviceInfo ConstructGraphicsDeviceInfo(NS::SharedPtr<MTL::Device> device);
    void InitRenderPassDescriptor(MTL::RenderPassColorAttachmentDescriptor* metalDescriptor, RenderPassRenderTarget* renderTargetDescriptor);

    // TODO: Move that function to common code (abstract shader for all graphics API)
    uint64_t ComputeRenderPipelineStateHash(MetalShader* shader, RenderPassDescriptor* renderPassDescriptor);
    MetalPipelineStateCacheItem* CreateRenderPipelineState(MetalShader* shader, RenderPassDescriptor* renderPassDescriptor);
};

static void MetalDeletePipelineCacheItem(uint64_t key, void* data);