#pragma once

typedef struct
{
    int32_t AlbedoTextureId;
    int32_t NormalTextureId;
    float4 AlbedoFactor;
    float3 EmissiveFactor;
} ShaderMaterial;

typedef struct
{
    int32_t MeshBufferIndex;
    float4 Rotation;
    float3 Translation;
    float Scale;
} GpuMeshInstance;

typedef struct 
{
    int32_t MeshInstanceId;
    int32_t MeshPrimitiveId;
} GpuMeshPrimitiveInstance;

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
    uint32_t TextureSampler;
    uint32_t Action;
} ShaderShaderGlobalParameters;
