#pragma once
#include <stdbool.h>
#include <stdint.h>

struct Vector4
{
    float X, Y, Z, W;
};

struct NullableVector4
{
    bool HasValue;
    struct Vector4 Value;
};

struct NullablePointer
{
    bool HasValue;
    void* Value;
};

enum GraphicsApi
{
    GraphicsApi_Unknown = 0,
    GraphicsApi_Direct3D12 = 1,
    GraphicsApi_Vulkan = 2,
    GraphicsApi_Metal = 3
};

enum ShaderStage
{
    ShaderStage_AmplificationShader = 1,
    ShaderStage_MeshShader = 2,
    ShaderStage_PixelShader = 3
};

enum ShaderMetaDataType
{
    ShaderMetaDataType_PushConstantsCount,
    ShaderMetaDataType_ThreadCountX,
    ShaderMetaDataType_ThreadCountY,
    ShaderMetaDataType_ThreadCountZ
};

struct ShaderMetaData
{
    enum ShaderMetaDataType Type;
    uint32_t Value;
};