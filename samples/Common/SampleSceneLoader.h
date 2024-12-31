#pragma once

#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleScene.h"
#include "SampleTexture.h"
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
    const char* Path;
    SampleTextureHeader TextureHeader;
    SampleTextureDataBlockEntry* MipDataEntries;
    SampleGpuTexture GpuTexture;
    bool IsLoaded;
} SampleTextureData;

typedef struct 
{
    SampleSceneMaterialHeader MaterialHeader;
    SampleTextureData AlbedoTexture;
    SampleTextureData NormalTexture;
} SampleMaterialData;

typedef struct
{
    uint32_t MeshCount;
    SampleMeshData* Meshes;
    uint32_t MaterialCount;
    SampleMaterialData* Materials;
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

void SampleLoadTexture(const char* path, SampleTextureData* textureData)
{
    *textureData = (SampleTextureData){};

    // TODO: Do better here
    char* tmp = malloc(strlen(path));
    strcpy(tmp, path);
    textureData->Path = tmp;

    FILE* file = SampleOpenFile(path, true);
    assert(file);
    
    fread(&textureData->TextureHeader, sizeof(SampleTextureHeader), 1, file);

    textureData->MipDataEntries = (SampleTextureDataBlockEntry*)malloc(sizeof(SampleTextureDataBlockEntry) * textureData->TextureHeader.MipCount);
    fread(textureData->MipDataEntries, sizeof(SampleTextureDataBlockEntry), textureData->TextureHeader.MipCount, file);

    fclose(file);
}

// TODO: In the future we will need to load individual mips from disk
void SampleLoadTextureData(SampleTextureData* textureData, SampleGpuMemory* gpuMemory)
{
    assert(textureData->Path);

    // TODO: Get texture name without folder and extension
    // TODO: Get the correct format
    textureData->GpuTexture = SampleCreateGpuTexture(gpuMemory, textureData->TextureHeader.Width, textureData->TextureHeader.Height, textureData->TextureHeader.MipCount, ElemGraphicsFormat_BC7_SRGB, textureData->Path);

    FILE* file = SampleOpenFile(textureData->Path, true);
    assert(file);

    //fseek(file, meshData->MeshHeader.MeshBufferOffset, SEEK_SET);

    fclose(file);

    textureData->IsLoaded = true;
}

void SampleFreeTexture(SampleTextureData* textureData)
{
    if (textureData->GpuTexture.Texture != ELEM_HANDLE_NULL)
    {
        SampleFreeGpuTexture(&textureData->GpuTexture);
    }
    // TODO: Free mallocs
}

void SampleLoadMaterial(const SampleSceneMaterialHeader* materialHeader, SampleMaterialData* materialData, const char* directoryPath)
{
    *materialData = (SampleMaterialData){};
    materialData->MaterialHeader = *materialHeader;

    // TODO: Some materials can use the same texture so we need to load it only once
    if (strlen(materialHeader->AlbedoTexturePath) > 0)
    {
        char fullTexturePath[MAX_PATH];
        strcpy(fullTexturePath, directoryPath);
        strcat(fullTexturePath, materialHeader->AlbedoTexturePath);

        SampleLoadTexture(fullTexturePath, &materialData->AlbedoTexture);
    }

    // TODO: NormalMap
}

void SampleFreeMaterial(SampleMaterialData* materialData)
{
    if (materialData->AlbedoTexture.IsLoaded)
    {
        SampleFreeTexture(&materialData->AlbedoTexture);
    }
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
    
    printf("Scene Loaded: Meshes Count=%d, Material Count=%d, Nodes Count=%d\n", sceneHeader.MeshCount, sceneHeader.MaterialCount, sceneHeader.NodeCount);

    sceneData->MeshCount = sceneHeader.MeshCount;
    sceneData->MaterialCount = sceneHeader.MaterialCount;
    sceneData->NodeCount = sceneHeader.NodeCount;

    // TODO: Replace malloc with an utility function that we will replace
    sceneData->Meshes = (SampleMeshData*)malloc(sizeof(SampleMeshData) * sceneHeader.MeshCount);

    SampleDataBlockEntry* meshDataBlocks = (SampleDataBlockEntry*)malloc(sizeof(SampleDataBlockEntry) * sceneHeader.MeshCount);
    fread(meshDataBlocks, sizeof(SampleDataBlockEntry), sceneHeader.MeshCount, file);

    sceneData->Materials = (SampleMaterialData*)malloc(sizeof(SampleMaterialData) * sceneHeader.MaterialCount);
    SampleSceneMaterialHeader* materialHeaders = (SampleSceneMaterialHeader*)malloc(sizeof(SampleSceneMaterialHeader) * sceneHeader.MaterialCount);
    fread(materialHeaders, sizeof(SampleSceneMaterialHeader), sceneHeader.MaterialCount, file);

    sceneData->Nodes = (SampleSceneNodeHeader*)malloc(sizeof(SampleSceneNodeHeader) * sceneHeader.NodeCount);
    fread(sceneData->Nodes, sizeof(SampleSceneNodeHeader), sceneHeader.NodeCount, file);

    fclose(file);

    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleLoadMesh(path, meshDataBlocks[i].Offset, &sceneData->Meshes[i]);
    }

    char directoryPath[MAX_PATH];
    GetFileDirectory(path, directoryPath, MAX_PATH);

    for (uint32_t i = 0; i < sceneData->MaterialCount; i++)
    {
        SampleLoadMaterial(&materialHeaders[i], &sceneData->Materials[i], directoryPath);
    }

    free(materialHeaders);
    free(meshDataBlocks);
}

void SampleFreeScene(const SampleSceneData* sceneData)
{
    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleFreeMesh(&sceneData->Meshes[i]);
    }

    for (uint32_t i = 0; i < sceneData->MaterialCount; i++)
    {
        SampleFreeMaterial(&sceneData->Materials[i]);
    }
}
