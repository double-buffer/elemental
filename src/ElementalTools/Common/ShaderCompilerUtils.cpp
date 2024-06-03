#include "ShaderCompilerUtils.h"

Span<uint8_t> CombineShaderParts(MemoryArena memoryArena, ReadOnlySpan<ShaderPart> shaderParts)
{
    // TODO: Embed shader type
    // TODO: Copy buffer in case of lib

    // HACK: This is normally temporary. In the future, DX12 should be able to manage only DXIL libs for programs
    if (shaderParts.Length > 1)
    {
        auto dataSize = 0u;

        for (uint32_t i = 1; i < shaderParts.Length; i++)
        {
            dataSize += shaderParts[i].ShaderCode.Length;
        }

        auto outputShaderData = SystemPushArray<uint8_t>(memoryArena, sizeof(uint32_t) * shaderParts.Length + dataSize);
        *(uint32_t*)outputShaderData.Pointer = shaderParts.Length - 1;
        
        uint32_t currentOffset = sizeof(uint32_t);

        for (uint32_t i = 1; i < shaderParts.Length; i++)
        {
            *(uint32_t*)outputShaderData.Slice(currentOffset).Pointer = shaderParts[i].ShaderCode.Length; 
            currentOffset += sizeof(uint32_t);

            SystemCopyBuffer((Span<uint8_t>)outputShaderData.Slice(currentOffset), (ReadOnlySpan<uint8_t>)shaderParts[i].ShaderCode);
            currentOffset += shaderParts[i].ShaderCode.Length;
        }

        return outputShaderData;
    }

    auto shaderPart = shaderParts[0];

    auto outputShaderData = SystemPushArray<uint8_t>(memoryArena, shaderPart.ShaderCode.Length);
    SystemCopyBuffer(outputShaderData, (ReadOnlySpan<uint8_t>)shaderPart.ShaderCode);
    return outputShaderData;
}
