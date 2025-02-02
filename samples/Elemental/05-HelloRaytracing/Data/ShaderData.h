#pragma once

#include "../../../Common/SampleShaderData.h"

typedef struct
{
    uint32_t GlobalParametersBufferIndex;
    uint32_t MeshPrimitiveInstanceId;
} ShaderParameters;

typedef struct
{
    float4x4 ViewProjMatrix;
    float4x4 InverseViewMatrix;
    float4x4 InverseProjectionMatrix;
    uint32_t MaterialBufferIndex;
    uint32_t MeshInstanceBufferIndex;
    uint32_t MeshPrimitiveInstanceBufferIndex;
    uint32_t Action;
} ShaderGlobalParameters;
