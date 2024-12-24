#pragma once

#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleScene.h"
#include "SampleGpuMemory.h"

typedef struct 
{
    uint32_t MeshPartCount;
    SampleMeshPartHeader* MeshParts;
    SampleGpuBuffer MeshBuffer;
} SampleMeshData;

typedef struct
{
    uint32_t MeshCount;
    SampleMeshData* Meshes;
} SampleSceneData;

void SampleLoadMesh(FILE* file, const SampleDataBlockEntry* datablock, SampleGpuMemory* gpuMemory, SampleMeshData* meshData, const char* debugName)
{
    fseek(file, datablock->Offset, SEEK_SET);

    SampleMeshHeader meshHeader;
    fread(&meshHeader, sizeof(SampleMeshHeader), 1, file);

    meshData->MeshPartCount = meshHeader.MeshPartCount;
    meshData->MeshParts = (SampleMeshPartHeader*)malloc(sizeof(SampleMeshPartHeader) * meshHeader.MeshPartCount);
    fread(meshData->MeshParts, sizeof(SampleMeshPartHeader), meshHeader.MeshPartCount, file);

    uint32_t currentOffset = ftell(file) - datablock->Offset;
    uint32_t meshBufferSizeInBytes = datablock->SizeInBytes - currentOffset;
    assert(meshBufferSizeInBytes > 0);

    uint8_t* meshBufferData = (uint8_t*)malloc(sizeof(uint8_t) * meshBufferSizeInBytes);
    fread(meshBufferData, sizeof(uint8_t), meshBufferSizeInBytes, file);

    // TODO: When IOQueues are implemented, we only need to read the header, not the whole file!
    meshData->MeshBuffer = SampleCreateGpuBufferAndUploadData(gpuMemory, meshBufferData, meshBufferSizeInBytes, debugName);
}

void SampleFreeMesh(SampleMeshData* meshData)
{
    SampleFreeGpuBuffer(&meshData->MeshBuffer);

    // TODO: Free mallocs
}

void SampleLoadScene(const char* path, SampleGpuMemory* gpuMemory, SampleSceneData* sceneData)
{
    // TODO: When IOQueues are implemented, we only need to read the header, not the whole file!

    FILE* file = SampleOpenFile(path, true);
    assert(file);

    SampleSceneHeader sceneHeader;
    fread(&sceneHeader, sizeof(sceneHeader), 1, file);

    if (!(sceneHeader.FileId[0] == 'S' && sceneHeader.FileId[1] == 'C' && sceneHeader.FileId[2] == 'E' && sceneHeader.FileId[3] == 'N' && sceneHeader.FileId[4] == 'E'))
    {
        printf("ERROR: Wrong scene format\n");
    }
    
    printf("OK Meshes Count: %d\n", sceneHeader.MeshCount);

    sceneData->MeshCount = sceneHeader.MeshCount;

    // TODO: Replace malloc with an utility function that we will replace
    sceneData->Meshes = (SampleMeshData*)malloc(sizeof(SampleMeshData) * sceneHeader.MeshCount);

    SampleDataBlockEntry* meshDataBlocks = (SampleDataBlockEntry*)malloc(sizeof(SampleDataBlockEntry) * sceneHeader.MeshCount);
    fread(meshDataBlocks, sizeof(SampleDataBlockEntry), sceneHeader.MeshCount, file);

    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleLoadMesh(file, &meshDataBlocks[i], gpuMemory, &sceneData->Meshes[i], "Mesh");
    }

    fclose(file);
}

void SampleFreeScene(const SampleSceneData* sceneData)
{
    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleFreeMesh(&sceneData->Meshes[i]);
    }
}
