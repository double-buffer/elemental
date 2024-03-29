#include "VulkanShader.h"
#include "VulkanGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define VULKAN_MAX_LIBRARIES UINT16_MAX
#define VULKAN_MAX_PIPELINESTATES UINT16_MAX

SystemDataPool<VulkanShaderLibraryData, VulkanShaderLibraryDataFull> vulkanShaderLibraryPool;
SystemDataPool<VulkanPipelineStateData, VulkanPipelineStateDataFull> vulkanPipelineStatePool;

void InitVulkanShaderLibraryMemory()
{
    if (!vulkanShaderLibraryPool.Storage)
    {
        vulkanShaderLibraryPool = SystemCreateDataPool<VulkanShaderLibraryData, VulkanShaderLibraryDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_LIBRARIES);
        vulkanPipelineStatePool = SystemCreateDataPool<VulkanPipelineStateData, VulkanPipelineStateDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_PIPELINESTATES);
    }
}

VulkanShaderLibraryData* GetVulkanShaderLibraryData(ElemShaderLibrary shaderLibrary)
{
    return SystemGetDataPoolItem(vulkanShaderLibraryPool, shaderLibrary);
}

VulkanShaderLibraryDataFull* GetVulkanShaderLibraryDataFull(ElemShaderLibrary shaderLibrary)
{
    return SystemGetDataPoolItemFull(vulkanShaderLibraryPool, shaderLibrary);
}

VulkanPipelineStateData* GetVulkanPipelineStateData(ElemPipelineState pipelineState)
{
    return SystemGetDataPoolItem(vulkanPipelineStatePool, pipelineState);
}

VulkanPipelineStateDataFull* GetVulkanPipelineStateDataFull(ElemPipelineState pipelineState)
{
    return SystemGetDataPoolItemFull(vulkanPipelineStatePool, pipelineState);
}

ElemShaderLibrary VulkanCreateShaderLibrary(ElemDataSpan shaderLibraryData)
{
    return ELEM_HANDLE_NULL;
}

void VulkanFreeShaderLibrary(ElemShaderLibrary shaderLibrary)
{
}

ElemPipelineState VulkanCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters)
{
    return ELEM_HANDLE_NULL;
}

void VulkanFreePipelineState(ElemPipelineState pipelineState)
{
}
