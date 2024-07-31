#include "ShaderReader.h"

template<typename T>
T ReadShaderData(ReadOnlySpan<uint8_t> data, uint32_t* currentOffset)
{
    auto dataSpan = Span<uint8_t>((uint8_t*)data.Pointer, data.Length);
    auto result = *(T*)dataSpan.Slice(*currentOffset).Pointer; 
    *currentOffset += sizeof(T);
    return result;
}

Span<uint8_t> ReadShaderData(MemoryArena memoryArena, ReadOnlySpan<uint8_t> data, uint32_t* currentOffset)
{
    auto shaderPartSizeInBytes = ReadShaderData<uint32_t>(data, currentOffset);
    auto result = SystemPushArray<uint8_t>(memoryArena, shaderPartSizeInBytes);

    auto dataSpan = Span<uint8_t>((uint8_t*)data.Pointer, data.Length);
    SystemCopyBuffer(result, ReadOnlySpan<uint8_t>(dataSpan.Slice(*currentOffset, shaderPartSizeInBytes))); 
    *currentOffset += shaderPartSizeInBytes;
    return result;
}

ReadOnlySpan<char> ReadShaderStringData(MemoryArena memoryArena, ReadOnlySpan<uint8_t> data, uint32_t* currentOffset)
{
    auto stringLength = ReadShaderData<uint32_t>(data, currentOffset);
    auto result = SystemPushArray<uint8_t>(memoryArena, stringLength);

    auto dataSpan = Span<uint8_t>((uint8_t*)data.Pointer, data.Length);
    SystemCopyBuffer(result, ReadOnlySpan<uint8_t>(dataSpan.Slice(*currentOffset, stringLength))); 
    *currentOffset += stringLength;
    return ReadOnlySpan<char>((char*)result.Pointer);
}

ReadOnlySpan<Shader> ReadShaders(MemoryArena memoryArena, ReadOnlySpan<uint8_t> data)
{
    uint32_t currentOffset = 0;

    char signature[9] = { };

    for (uint32_t i = 0; i < ARRAYSIZE(signature) - 1; i++)
    {
        signature[i] = ReadShaderData<char>(data, &currentOffset);
    }

    if (!strstr(signature, "ELEMSLIB"))
    {
        return {}; 
    }

    auto shaderPartsCount = ReadShaderData<uint32_t>(data, &currentOffset);
    auto shaderParts = SystemPushArray<Shader>(memoryArena, shaderPartsCount);

    for (uint32_t i = 0; i < shaderPartsCount; i++)
    {
        shaderParts[i].ShaderType = ReadShaderData<ShaderType>(data, &currentOffset);
        auto name = ReadShaderStringData(memoryArena, data, &currentOffset);

        auto metadataLength = ReadShaderData<uint32_t>(data, &currentOffset);
        auto metadata = SystemPushArray<ShaderMetadata>(memoryArena, metadataLength);

        for (uint32_t j = 0; j < metadataLength; j++)
        {
            metadata[j].Type = ReadShaderData<ShaderMetadataType>(data, &currentOffset);

            for (uint32_t k = 0; k < ARRAYSIZE(metadata[j].Value); k++)
            {
                metadata[j].Value[k] = ReadShaderData<uint32_t>(data, &currentOffset);
            }
        }

        shaderParts[i].Name = name;
        shaderParts[i].Metadata = metadata;
        shaderParts[i].ShaderCode = ReadShaderData(memoryArena, data, &currentOffset);
    }

    return shaderParts;
}

