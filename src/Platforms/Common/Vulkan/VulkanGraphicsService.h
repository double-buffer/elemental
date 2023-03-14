#pragma once

#include "../BaseGraphicsService.h"
#include "../BaseGraphicsObject.h"
#include "../SystemFunctions.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#include "volk.h"

struct VulkanGraphicsDevice;

#define MAX_VULKAN_GRAPHICS_DEVICES 64
#define MAX_VULKAN_COMMAND_POOLS 64
#define MAX_VULKAN_COMMAND_BUFFERS 64

#include "VulkanBaseGraphicsObject.h"
#include "VulkanCommandQueue.h"
#include "VulkanCommandList.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanSwapChain.h"
#include "VulkanGraphicsDevice.h"

class VulkanGraphicsService : BaseGraphicsService
{
public:
    VulkanGraphicsService(GraphicsServiceOptions* options);
    ~VulkanGraphicsService();

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
    GraphicsDiagnostics _graphicsDiagnostics;
    VkInstance _vulkanInstance = nullptr;
    VkDebugReportCallbackEXT _debugCallback = nullptr;
    uint32_t _currentDeviceInternalId = 0;

    GraphicsDeviceInfo ConstructGraphicsDeviceInfo(VkPhysicalDeviceProperties deviceProperties, VkPhysicalDeviceMemoryProperties deviceMemoryProperties);
    VkDeviceQueueCreateInfo CreateDeviceQueueCreateInfo(uint32_t queueFamilyIndex, uint32_t count);

    VulkanCommandPoolItem* GetCommandPool(VulkanCommandQueue* commandQueue);
    void UpdateCommandPoolFence(VulkanCommandList* commandList, uint64_t fenceValue);
    VulkanCommandList* GetCommandList(VulkanCommandQueue* commandQueue, VulkanCommandPoolItem* commandPoolItem);
    Fence CreateCommandQueueFence(VulkanCommandQueue* commandQueue);

    void CreateSwapChainBackBuffers(VulkanSwapChain* swapChain, int32_t width, int32_t height);
    
    uint64_t ComputeRenderPipelineStateHash(VulkanShader* shader, RenderPassDescriptor* renderPassDescriptor);
    VulkanPipelineStateCacheItem CreateRenderPipelineState(VulkanShader* shader, RenderPassDescriptor* renderPassDescriptor);

    void TransitionTextureToState(VulkanCommandList* commandList, VulkanTexture* texture, VkImageLayout sourceState, VkImageLayout destinationState, bool isTransfer = false);
};

static VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage, void *pUserData);

thread_local VulkanDeviceCommandPools CommandPools[MAX_VULKAN_GRAPHICS_DEVICES];