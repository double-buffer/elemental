#pragma once

#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleScene.h"
#include "SampleTexture.h"
#include "SampleGpuMemory.h"

// TODO: Refactor those 3 structs
typedef struct 
{
    const char* Path;
    SampleMeshHeader MeshHeader;
    SampleMeshPrimitiveHeader* MeshPrimitives;
} SampleMeshData;

typedef struct
{
    const char* Path;
    SampleTextureHeader TextureHeader;
    SampleTextureDataBlockEntry* MipDataEntries;
    bool IsNormalTexture;
} SampleTextureData;

typedef struct
{
    uint32_t MeshCount;
    SampleMeshData* Meshes;
    uint32_t TextureCount;
    SampleTextureData* Textures;
    uint32_t MaterialCount;
    SampleSceneMaterialHeader* Materials;
    uint32_t NodeCount;
    SampleSceneNodeHeader* Nodes;
} SampleSceneData;


// TODO: This should be removed and the struct is specific per sample
// it should be shared with an header file between c code and shaders
typedef struct
{
    int32_t AlbedoTextureId;
    int32_t NormalTextureId;
    ElemVector4 AlbedoFactor;
    ElemVector3 EmissiveFactor;
} ShaderMaterial;

typedef struct
{
    int32_t MeshBufferIndex;
    ElemVector4 Rotation;
    ElemVector3 Translation;
    float Scale;
} GpuMeshInstance;

typedef struct 
{
    int32_t MeshInstanceId;
    int32_t MeshPrimitiveId;
} GpuMeshPrimitiveInstance;


void SampleLoadMesh(const char* path, uint32_t offset, SampleMeshData* meshData, SampleGpuMemory* gpuMemory)
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

void SampleLoadTexture(const char* path, SampleTextureData* textureData, bool isNormalTexture)
{
    *textureData = (SampleTextureData){};

    printf("Loading Texture: %s\n", path);
    // TODO: Do better here
    char* tmp = malloc(strlen(path) + 1);
    strcpy(tmp, path);
    textureData->Path = tmp;
    textureData->IsNormalTexture = isNormalTexture;

    FILE* file = SampleOpenFile(path, true);

    if (!file)
    {
        printf("NO TEXT: %s\n", path);
        return;
    }
    assert(file);
    
    fread(&textureData->TextureHeader, sizeof(SampleTextureHeader), 1, file);

    textureData->MipDataEntries = (SampleTextureDataBlockEntry*)malloc(sizeof(SampleTextureDataBlockEntry) * textureData->TextureHeader.MipCount);
    fread(textureData->MipDataEntries, sizeof(SampleTextureDataBlockEntry), textureData->TextureHeader.MipCount, file);

    fclose(file);
}

// TODO: IMPORTANT: Don't fill the gpu buffers here. It should be the caller code that do that because we don't know
// which strategy the caller will use to manage his buffers. Here we just need provide access to the data
void SampleLoadScene(const char* path, SampleSceneData* sceneData, SampleGpuMemory* gpuMemory)
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
    
    printf("Scene Loaded: Meshes Count=%d, Material Count=%d, Texture Count=%d, Nodes Count=%d\n", sceneHeader.MeshCount, sceneHeader.MaterialCount, sceneHeader.TextureCount, sceneHeader.NodeCount);

    sceneData->MeshCount = sceneHeader.MeshCount;
    sceneData->TextureCount = sceneHeader.TextureCount;
    sceneData->MaterialCount = sceneHeader.MaterialCount;
    sceneData->NodeCount = sceneHeader.NodeCount;

    // TODO: Replace malloc with an utility function that we will replace
    sceneData->Meshes = (SampleMeshData*)malloc(sizeof(SampleMeshData) * sceneHeader.MeshCount);

    SampleDataBlockEntry* meshDataBlocks = (SampleDataBlockEntry*)malloc(sizeof(SampleDataBlockEntry) * sceneHeader.MeshCount);
    fread(meshDataBlocks, sizeof(SampleDataBlockEntry), sceneHeader.MeshCount, file);

    sceneData->Textures = (SampleTextureData*)malloc(sizeof(SampleTextureData) * sceneHeader.TextureCount);
    SampleSceneTextureData* textureHeaders = (SampleSceneTextureData*)malloc(sizeof(SampleSceneTextureData) * sceneHeader.TextureCount);
    fread(textureHeaders, sizeof(SampleSceneTextureData), sceneHeader.TextureCount, file);

    sceneData->Materials = (SampleSceneMaterialHeader*)malloc(sizeof(SampleSceneMaterialHeader) * sceneHeader.MaterialCount);
    fread(sceneData->Materials, sizeof(SampleSceneMaterialHeader), sceneHeader.MaterialCount, file);

    sceneData->Nodes = (SampleSceneNodeHeader*)malloc(sizeof(SampleSceneNodeHeader) * sceneHeader.NodeCount);
    fread(sceneData->Nodes, sizeof(SampleSceneNodeHeader), sceneHeader.NodeCount, file);

    fclose(file);

    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleLoadMesh(path, meshDataBlocks[i].Offset, &sceneData->Meshes[i], gpuMemory);
    }

    char directoryPath[MAX_PATH];
    GetFileDirectory(path, directoryPath, MAX_PATH);

    for (uint32_t i = 0; i < sceneData->TextureCount; i++)
    {
        SampleSceneTextureData* textureHeader = &textureHeaders[i];
        SampleTextureData* textureData = &sceneData->Textures[i];

        char fullTexturePath[MAX_PATH];
        memset(fullTexturePath, 0, MAX_PATH);
        strcpy(fullTexturePath, directoryPath);
        strcat(fullTexturePath, textureHeader->Path);

        SampleLoadTexture(fullTexturePath, textureData, textureHeader->IsNormalTexture);
    }

    free(meshDataBlocks);
}

void SampleFreeScene(SampleSceneData* sceneData)
{
    // TODO: Free mallocs
}
