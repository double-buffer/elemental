#pragma once

#include <stdint.h>

typedef struct
{
    char FileId[4];
    uint32_t MeshletCount;
    uint8_t MeshletMaxVertexCount;
    uint8_t MeshletMaxTriangleCount;
    uint32_t VertexBufferOffset;
    uint32_t VertexBufferSizeInBytes;
    uint32_t MeshletBufferOffset;
    uint32_t MeshletBufferSizeInBytes;
    uint32_t MeshletVertexIndexBufferOffset;
    uint32_t MeshletVertexIndexBufferSizeInBytes;
    uint32_t MeshletTriangleIndexBufferOffset;
    uint32_t MeshletTriangleIndexBufferSizeInBytes;
} SampleMeshHeader;
