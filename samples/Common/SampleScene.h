#pragma once

#include "SampleMath.h"
#include <stdint.h>

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
} SampleSceneHeader;

typedef struct
{
    char Name[50];
    SampleSceneNodeType NodeType;
    int32_t ReferenceIndex;
    ElemVector3 Rotation;
    ElemVector3 Translation;
    // TODO: Children
} SampleSceneNodeHeader;

typedef struct
{
    // TODO: Vertex size, etc.
    uint32_t MeshPrimitiveCount;
    uint32_t MeshBufferOffset;
    uint32_t MeshBufferSizeInBytes;
} SampleMeshHeader;

// TODO: Rename to mesh primitive?
typedef struct
{
    uint32_t MeshletCount;
    uint32_t VertexBufferOffset;
    uint32_t MeshletOffset;
    uint32_t MeshletVertexIndexOffset;
    uint32_t MeshletTriangleIndexOffset;
} SampleMeshPrimitiveHeader;

typedef struct
{
    uint32_t Offset;
    uint32_t SizeInBytes;
} SampleDataBlockEntry;
