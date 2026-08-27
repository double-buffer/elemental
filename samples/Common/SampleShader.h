#pragma once

#include "Elemental.h"
#include "SampleUtils.h"

typedef struct
{
    ElemPipelineState PipelineState;
} SampleShader;

ElemDataSpan SampleLoadShaderData(ElemGraphicsDevice graphicsDevice, const char* shaderFile)
{
    ElemGraphicsDeviceInfo graphicsDeviceInfo = ElemGetGraphicsDeviceInfo(graphicsDevice);

    if (graphicsDeviceInfo.GraphicsApi != ElemGraphicsApi_Vulkan)
    {
        return SampleReadFile(shaderFile, true);
    }
    else
    {
        char vulkanPath[255];
        ReplaceFileExtension(shaderFile, "_vulkan.shader", vulkanPath, 255);

        return SampleReadFile(vulkanPath, true);
    }
}

SampleShader SampleCompileGraphicsShader(ElemGraphicsDevice graphicsDevice, const char* shaderFile, ElemGraphicsPipelineStateParameters* parameters)
{
    ElemDataSpan shaderData = SampleLoadShaderData(graphicsDevice, shaderFile);
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(graphicsDevice, shaderData);

    parameters->ShaderLibrary = shaderLibrary;

    ElemPipelineState pipelineState = ElemCompileGraphicsPipelineState(graphicsDevice, parameters);

    ElemFreeShaderLibrary(shaderLibrary);

    return (SampleShader) { .PipelineState = pipelineState };
}

SampleShader SampleCompileComputeShader(ElemGraphicsDevice graphicsDevice, const char* shaderFile, ElemComputePipelineStateParameters* parameters)
{
    ElemDataSpan shaderData = SampleLoadShaderData(graphicsDevice, shaderFile);
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(graphicsDevice, shaderData);

    parameters->ShaderLibrary = shaderLibrary;

    ElemPipelineState pipelineState = ElemCompileComputePipelineState(graphicsDevice, parameters);

    ElemFreeShaderLibrary(shaderLibrary);

    return (SampleShader) { .PipelineState = pipelineState };
}

void SampleFreeShader(const SampleShader* shader)
{
    ElemFreePipelineState(shader->PipelineState);
}
