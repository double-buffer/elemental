#pragma once

#include "Elemental.h"
#include "SystemSpan.h"
#include "volk.h"

struct VulkanShaderLibraryData
{
    ReadOnlySpan<VkShaderModule> GraphicsShaders;
};

struct VulkanShaderLibraryDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

struct VulkanPipelineStateData
{
    VkPipeline PipelineState;
    ElemGraphicsDevice GraphicsDevice;
};

struct VulkanPipelineStateDataFull
{
    uint32_t reserved;
};

VulkanShaderLibraryData* GetVulkanShaderLibraryData(ElemShaderLibrary shaderLibrary);
VulkanShaderLibraryDataFull* GetVulkanShaderLibraryDataFull(ElemTexture shaderLibrary);

VulkanPipelineStateData* GetVulkanPipelineStateData(ElemPipelineState pipelineState);
VulkanPipelineStateDataFull* GetVulkanPipelineStateDataFull(ElemPipelineState pipelineState);

ElemShaderLibrary VulkanCreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData);
void VulkanFreeShaderLibrary(ElemShaderLibrary shaderLibrary);
ElemPipelineState VulkanCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters);
ElemPipelineState VulkanCompileComputePipelineState(ElemGraphicsDevice graphicsDevice, const ElemComputePipelineStateParameters* parameters);
void VulkanFreePipelineState(ElemPipelineState pipelineState);
void VulkanBindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState);
void VulkanPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data); 

void VulkanDispatchCompute(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
