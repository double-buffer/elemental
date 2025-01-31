#pragma once

#include "SampleMath.h"
#include <stdint.h>

// TODO: Rename *Header by data when we have refactored the sceneloader
typedef enum
{
    SampleSceneNodeType_Unknown = 0,
    SampleSceneNodeType_Mesh = 1
} SampleSceneNodeType;

typedef struct
{
    char FileId[5];
    uint32_t MeshCount;
    uint32_t NodeCount;
    uint32_t MaterialCount;
    uint32_t TextureCount;
} SampleSceneHeader;

typedef struct
{
    char Name[50];
    char AlbedoTexturePath[255];
    char NormalTexturePath[255];
    int32_t AlbedoTextureId;
    int32_t NormalTextureId;
    ElemVector4 AlbedoFactor;
    ElemVector3 EmissiveFactor;
} SampleSceneMaterialHeader;

typedef struct
{
    char Path[255];
    bool IsNormalTexture;
} SampleSceneTextureData;

typedef struct
{
    char Name[50];
    SampleSceneNodeType NodeType;
    int32_t ReferenceIndex;
    ElemVector4 Rotation;
    float Scale;
    ElemVector3 Translation;
    // TODO: Children
} SampleSceneNodeHeader;

typedef struct
{
    char Name[50];
    // TODO: Vertex size, etc.
    uint32_t VertexSizeInBytes;
    uint32_t MeshPrimitiveCount;
    uint32_t MeshBufferOffset;
    uint32_t MeshBufferSizeInBytes;
} SampleMeshHeader;

typedef struct
{
    uint32_t MeshletOffset;
    uint32_t MeshletCount;
    uint32_t VertexBufferOffset;
    uint32_t VertexCount;
    uint32_t IndexBufferOffset;
    uint32_t IndexCount;
    int32_t MaterialId;
} SampleMeshPrimitiveHeader;

typedef struct
{
    uint32_t Offset;
    uint32_t SizeInBytes;
} SampleDataBlockEntry;
