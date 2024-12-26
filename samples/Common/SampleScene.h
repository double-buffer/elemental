#pragma once

#include <stdint.h>

typedef struct
{
    char FileId[5];
    uint32_t MeshCount;
} SampleSceneHeader;

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
