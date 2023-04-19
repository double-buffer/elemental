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

typedef enum
{
    GraphicsApi_Unknown = 0,
    GraphicsApi_Direct3D12 = 1,
    GraphicsApi_Vulkan = 2,
    GraphicsApi_Metal = 3
} GraphicsApi;

typedef enum
{
    ShaderStage_AmplificationShader = 1,
    ShaderStage_MeshShader = 2,
    ShaderStage_PixelShader = 3
} ShaderStage;

typedef enum
{
    ShaderMetaDataType_PushConstantsCount,
    ShaderMetaDataType_ThreadCountX,
    ShaderMetaDataType_ThreadCountY,
    ShaderMetaDataType_ThreadCountZ
} ShaderMetaDataType;

typedef struct _ShaderMetaData
{
    ShaderMetaDataType Type;
    uint32_t Value;
} ShaderMetaData;