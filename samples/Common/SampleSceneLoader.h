#pragma once

#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleScene.h"
#include "SampleGpuMemory.h"

// TODO: The struct here are temporary. The final data will be placed in buffer
typedef struct 
{
    const char* Path;
    SampleMeshHeader MeshHeader;
    SampleMeshPrimitiveHeader* MeshPrimitives;
    SampleGpuBuffer MeshBuffer;
} SampleMeshData;

typedef struct
{
    uint32_t MeshCount;
    SampleMeshData* Meshes;
    uint32_t NodeCount;
    SampleSceneNodeHeader* Nodes;
} SampleSceneData;

void SampleLoadMesh(const char* path, uint32_t offset, SampleMeshData* meshData)
{
    *meshData = (SampleMeshData){};
    meshData->Path = path;

    FILE* file = SampleOpenFile(path, true);
    assert(file);

    if (offset > 0)
    {
        fseek(file, offset, SEEK_SET);
    }
    
    fread(&meshData->MeshHeader, sizeof(SampleMeshHeader), 1, file);

    // TODO: Decode header
    // TODO: Can we get rid of the malloc?

    meshData->MeshPrimitives = (SampleMeshPrimitiveHeader*)malloc(sizeof(SampleMeshPrimitiveHeader) * meshData->MeshHeader.MeshPrimitiveCount);
    fread(meshData->MeshPrimitives, sizeof(SampleMeshPrimitiveHeader), meshData->MeshHeader.MeshPrimitiveCount, file);

    fclose(file);
}

// TODO: We should be able to replace this whole function with IO Queues
void SampleLoadMeshData(SampleMeshData* meshData, SampleGpuMemory* gpuMemory)
{
    assert(meshData->Path);

    FILE* file = SampleOpenFile(meshData->Path, true);
    assert(file);

    fseek(file, meshData->MeshHeader.MeshBufferOffset, SEEK_SET);

    uint8_t* meshBufferData = (uint8_t*)malloc(sizeof(uint8_t) * meshData->MeshHeader.MeshBufferSizeInBytes);
    fread(meshBufferData, sizeof(uint8_t), meshData->MeshHeader.MeshBufferSizeInBytes, file);

    // TODO: Construct debug name
    meshData->MeshBuffer = SampleCreateGpuBufferAndUploadData(gpuMemory, meshBufferData, meshData->MeshHeader.MeshBufferSizeInBytes, meshData->MeshHeader.Name);

    fclose(file);
}

void SampleFreeMesh(SampleMeshData* meshData)
{
    if(meshData->MeshBuffer.Buffer != ELEM_HANDLE_NULL)
    {
        SampleFreeGpuBuffer(&meshData->MeshBuffer);
    }

    // TODO: Free mallocs
}

void SampleLoadScene(const char* path, SampleSceneData* sceneData)
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
    
    printf("Scene Loaded: Meshes Count=%d, Nodes Count=%d\n", sceneHeader.MeshCount, sceneHeader.NodeCount);

    sceneData->MeshCount = sceneHeader.MeshCount;
    sceneData->NodeCount = sceneHeader.NodeCount;

    // TODO: Replace malloc with an utility function that we will replace
    sceneData->Meshes = (SampleMeshData*)malloc(sizeof(SampleMeshData) * sceneHeader.MeshCount);

    SampleDataBlockEntry* meshDataBlocks = (SampleDataBlockEntry*)malloc(sizeof(SampleDataBlockEntry) * sceneHeader.MeshCount);
    fread(meshDataBlocks, sizeof(SampleDataBlockEntry), sceneHeader.MeshCount, file);

    sceneData->Nodes = (SampleSceneNodeHeader*)malloc(sizeof(SampleSceneNodeHeader) * sceneHeader.NodeCount);
    fread(sceneData->Nodes, sizeof(SampleSceneNodeHeader), sceneHeader.NodeCount, file);

    fclose(file);

    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleLoadMesh(path, meshDataBlocks[i].Offset, &sceneData->Meshes[i]);
        //SampleLoadMeshData(file, gpuMemory, &sceneData->Meshes[i], "Mesh");
    }

    free(meshDataBlocks);
}

void SampleFreeScene(const SampleSceneData* sceneData)
{
    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleFreeMesh(&sceneData->Meshes[i]);
    }
}
