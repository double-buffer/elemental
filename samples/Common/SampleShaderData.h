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
