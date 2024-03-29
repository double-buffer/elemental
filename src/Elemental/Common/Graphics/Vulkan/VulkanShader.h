#pragma once

#include "Elemental.h"

struct VulkanShaderLibraryData
{
};

struct VulkanShaderLibraryDataFull
{
};

struct VulkanPipelineStateData
{
};

struct VulkanPipelineStateDataFull
{
};

VulkanShaderLibraryData* GetVulkanShaderLibraryData(ElemShaderLibrary shaderLibrary);
VulkanShaderLibraryDataFull* GetVulkanShaderLibraryDataFull(ElemTexture shaderLibrary);

VulkanPipelineStateData* GetVulkanPipelineStateData(ElemPipelineState pipelineState);
VulkanPipelineStateDataFull* GetVulkanPipelineStateDataFull(ElemPipelineState pipelineState);

ElemShaderLibrary VulkanCreateShaderLibrary(ElemDataSpan shaderLibraryData);
void VulkanFreeShaderLibrary(ElemShaderLibrary shaderLibrary);
ElemPipelineState VulkanCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters);
void VulkanFreePipelineState(ElemPipelineState pipelineState);
void VulkanBindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState);
void VulkanPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data); 
