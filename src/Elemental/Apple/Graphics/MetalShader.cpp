#include "MetalShader.h"
#include "MetalGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define METAL_MAX_LIBRARIES UINT16_MAX
#define METAL_MAX_PIPELINESTATES UINT16_MAX

SystemDataPool<MetalShaderLibraryData, MetalShaderLibraryDataFull> metalShaderLibraryPool;
SystemDataPool<MetalPipelineStateData, MetalPipelineStateDataFull> metalPipelineStatePool;

void InitMetalShaderLibraryMemory()
{
    if (!metalShaderLibraryPool.Storage)
    {
        metalShaderLibraryPool = SystemCreateDataPool<MetalShaderLibraryData, MetalShaderLibraryDataFull>(MetalGraphicsMemoryArena, METAL_MAX_LIBRARIES);
        metalPipelineStatePool = SystemCreateDataPool<MetalPipelineStateData, MetalPipelineStateDataFull>(MetalGraphicsMemoryArena, METAL_MAX_PIPELINESTATES);
    }
}

MetalShaderLibraryData* GetMetalShaderLibraryData(ElemShaderLibrary shaderLibrary)
{
    return SystemGetDataPoolItem(metalShaderLibraryPool, shaderLibrary);
}

MetalShaderLibraryDataFull* GetMetalShaderLibraryDataFull(ElemShaderLibrary shaderLibrary)
{
    return SystemGetDataPoolItemFull(metalShaderLibraryPool, shaderLibrary);
}

MetalPipelineStateData* GetMetalPipelineStateData(ElemPipelineState pipelineState)
{
    return SystemGetDataPoolItem(metalPipelineStatePool, pipelineState);
}

MetalPipelineStateDataFull* GetMetalPipelineStateDataFull(ElemPipelineState pipelineState)
{
    return SystemGetDataPoolItemFull(metalPipelineStatePool, pipelineState);
}

ElemShaderLibrary MetalCreateShaderLibrary(ElemDataSpan shaderLibraryData)
{
    return ELEM_HANDLE_NULL;
}

void MetalFreeShaderLibrary(ElemShaderLibrary shaderLibrary)
{
}

ElemPipelineState MetalCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters)
{
    return ELEM_HANDLE_NULL;
}

void MetalFreePipelineState(ElemPipelineState pipelineState)
{
}

void MetalBindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState)
{
}

void MetalPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data)
{
}
