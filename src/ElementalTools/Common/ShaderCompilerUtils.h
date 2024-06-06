#pragma once

#include "SystemMemory.h"

enum ShaderType
{
    ShaderType_Amplification = 0,
    ShaderType_Mesh = 1,
    ShaderType_Pixel = 2,
    ShaderType_Compute = 3,
    ShaderType_Library = 4
};

struct ShaderPart
{
    ShaderType ShaderType;
    ReadOnlySpan<uint8_t> ShaderCode;
};

Span<uint8_t> CombineShaderParts(MemoryArena memoryArena, ReadOnlySpan<ShaderPart> shaderParts);
