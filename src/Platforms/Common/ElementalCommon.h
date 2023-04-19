#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    float X, Y, Z, W;
} Vector4;

typedef struct
{
    bool HasValue;
    Vector4 Value;
} NullableVector4;

typedef struct
{
    bool HasValue;
    void* Value;
} NullablePointer;

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

typedef struct _ShaderMetaData
{
    enum ShaderMetaDataType Type;
    uint32_t Value;
} ShaderMetaData;