#pragma once

#include "../../../Common/SampleShaderData.h"

typedef struct
{
    uint32_t GlobalParametersBufferIndex;
    uint32_t MeshPrimitiveInstanceId;
    uint32_t AccelerationStructureIndex;
} ShaderParameters;

typedef struct
{
    uint32_t AccelerationStructureIndex;
    uint32_t GlobalParametersBufferIndex;
    uint32_t OutputTextureIndex;
    uint32_t SampleCount;
    uint32_t FrameIndex;
    uint32_t PathTraceLength;
    uint32_t Reserved1;
    uint32_t Reserved2;
    float2 OutputTextureSize;
} RaytracingShaderParameters;

typedef struct 
{
    uint32_t TextBufferIndex;
    uint32_t Draw2DCommandsBufferIndex;
    uint32_t CommandCount;
    uint32_t Reserved1;
    float2 RenderTargetSize;
} DrawTextShaderParameters;

typedef enum
{
    Draw2DCommandType_Text
} Draw2DCommandType;

typedef struct
{
    uint32_t Type;
    uint32_t CommandDataOffset;
    float PositionX;
    float PositionY;
} Draw2DCommand;

typedef struct
{
    float4x4 ViewProjMatrix;
    float4x4 InverseViewMatrix;
    float4x4 InverseProjectionMatrix;
    uint32_t MaterialBufferIndex;
    uint32_t MeshInstanceBufferIndex;
    uint32_t MeshPrimitiveInstanceBufferIndex;
    uint32_t TextureSampler;
    uint32_t Action;
} ShaderGlobalParameters;
