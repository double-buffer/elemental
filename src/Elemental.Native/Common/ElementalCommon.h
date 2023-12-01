#pragma once

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
    LogMessageType_Debug = 0,
    LogMessageType_Warning = 1,
    LogMessageType_Error = 2
} LogMessageType;

typedef enum
{
    LogMessageCategory_Memory = 0,
    LogMessageCategory_NativeApplication = 1,
    LogMessageCategory_Graphics = 2,
    LogMessageCategory_Inputs = 3
} LogMessageCategory;

typedef void (*LogMessageHandlerPtr)(LogMessageType messageType, LogMessageCategory category, const char* function, const char* message);

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
