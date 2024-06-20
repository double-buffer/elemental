#include "ShaderCompilerUtils.h"

template<typename T>
void WriteShaderData(Span<uint8_t> data, uint32_t* currentOffset, T value)
{
    *(T*)data.Slice(*currentOffset).Pointer = value; 
    *currentOffset += sizeof(T);
}

template<>
void WriteShaderData(Span<uint8_t> data, uint32_t* currentOffset, ReadOnlySpan<uint8_t> value)
{
    SystemCopyBuffer(data.Slice(*currentOffset), value);
    *currentOffset += value.Length;
}

template<>
void WriteShaderData(Span<uint8_t> data, uint32_t* currentOffset, ReadOnlySpan<char> value)
{
    auto dataSpan = ReadOnlySpan<uint8_t>((uint8_t*)value.Pointer, value.Length);
    SystemCopyBuffer(data.Slice(*currentOffset), dataSpan);
    *currentOffset += value.Length + 1;
}

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

Span<uint8_t> CombineShaderParts(MemoryArena memoryArena, ReadOnlySpan<ShaderPart> shaderParts)
{
    auto dataSize = 8;
    dataSize += sizeof(uint32_t);

    for (uint32_t i = 0; i < shaderParts.Length; i++)
    {
        dataSize += sizeof(ShaderType);
        dataSize += sizeof(uint32_t);

        dataSize += sizeof(uint32_t);
        dataSize += shaderParts[i].Name.Length + 1;

        for (uint32_t j = 0; j < shaderParts[i].Metadata.Length; j++)
        {
            dataSize += sizeof(ShaderMetadataType);
            dataSize += 4 * sizeof(uint32_t);
        }

        dataSize += sizeof(uint32_t);
        dataSize += shaderParts[i].ShaderCode.Length;
    }

    auto outputShaderData = SystemPushArray<uint8_t>(memoryArena, dataSize);
    uint32_t currentOffset = 0;

    char signature[8] = { 'E', 'L', 'E', 'M', 'S', 'L', 'I', 'B' };

    for (uint32_t i = 0; i < ARRAYSIZE(signature); i++)
    {
        WriteShaderData(outputShaderData, &currentOffset, signature[i]);
    }

    WriteShaderData(outputShaderData, &currentOffset, (uint32_t)shaderParts.Length);

    for (uint32_t i = 0; i < shaderParts.Length; i++)
    {
        auto shaderPart = shaderParts[i];

        WriteShaderData(outputShaderData, &currentOffset, shaderPart.ShaderType);

        WriteShaderData(outputShaderData, &currentOffset, (uint32_t)shaderPart.Name.Length + 1);
        WriteShaderData(outputShaderData, &currentOffset, shaderPart.Name);

        WriteShaderData(outputShaderData, &currentOffset, (uint32_t)shaderPart.Metadata.Length);

        for (uint32_t j = 0; j < shaderPart.Metadata.Length; j++)
        {
            auto metadata = shaderPart.Metadata[j];
            WriteShaderData(outputShaderData, &currentOffset, metadata.Type);

            for (uint32_t k = 0; k < ARRAYSIZE(metadata.Value); k++)
            {
                WriteShaderData(outputShaderData, &currentOffset, metadata.Value[k]);
            }
        }

        WriteShaderData(outputShaderData, &currentOffset, (uint32_t)shaderPart.ShaderCode.Length);
        WriteShaderData(outputShaderData, &currentOffset, shaderPart.ShaderCode);
    }

    return outputShaderData;
}

ReadOnlySpan<ShaderPart> ReadShaderParts(MemoryArena memoryArena, ReadOnlySpan<uint8_t> data)
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
    auto shaderParts = SystemPushArray<ShaderPart>(memoryArena, shaderPartsCount);

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
