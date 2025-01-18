#pragma once

#include "SystemMemory.h"

enum ShaderType
{
    ShaderType_Unknown = 0,
    ShaderType_Mesh = 1,
    ShaderType_Pixel = 2,
    ShaderType_Compute = 3,
    ShaderType_Library = 4
};

enum ShaderMetadataType
{
    ShaderMetadataType_Unknown = 0,
    ShaderMetadataType_ThreadGroupSize = 1
};

struct ShaderMetadata
{
    ShaderMetadataType Type;
    uint32_t Value[4];
};

struct Shader
{
    ShaderType ShaderType;
    ReadOnlySpan<char> Name;
    ReadOnlySpan<ShaderMetadata> Metadata;
    ReadOnlySpan<uint8_t> ShaderCode;
};

ReadOnlySpan<Shader> ReadShaders(MemoryArena memoryArena, ReadOnlySpan<uint8_t> data);
