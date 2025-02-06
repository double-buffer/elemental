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
    float4x4 ViewProjMatrix;
    float4x4 InverseViewMatrix;
    float4x4 InverseProjectionMatrix;
    uint32_t MaterialBufferIndex;
    uint32_t MeshInstanceBufferIndex;
    uint32_t MeshPrimitiveInstanceBufferIndex;
    uint32_t TextureSampler;
    uint32_t Action;
} ShaderGlobalParameters;
