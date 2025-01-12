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

// TODO: Do otherwise
typedef struct
{
    int32_t AlbedoTextureId;
    int32_t NormalTextureId;
    ElemVector4 AlbedoFactor;
} ShaderMaterial;

typedef struct
{
    uint32_t MeshCount;
    SampleMeshData* Meshes;
    uint32_t MaterialCount;
    SampleMaterialData* Materials;
    uint32_t NodeCount;
    SampleSceneNodeHeader* Nodes;
    SampleGpuBuffer MaterialBuffer;
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
void SampleLoadMeshData(ElemCommandList commandList, SampleMeshData* meshData, SampleGpuMemory* gpuMemory)
{
    // TODO: Construct debug name
    meshData->MeshBuffer = SampleCreateGpuBuffer(gpuMemory, meshData->MeshHeader.MeshBufferSizeInBytes, meshData->MeshHeader.Name);

    // TODO: To replace with file IO 
    assert(meshData->Path);
    uint8_t* meshBufferData = (uint8_t*)malloc(sizeof(uint8_t) * meshData->MeshHeader.MeshBufferSizeInBytes);

    FILE* file = SampleOpenFile(meshData->Path, true);
    assert(file);

    fseek(file, meshData->MeshHeader.MeshBufferOffset, SEEK_SET);
    fread(meshBufferData, sizeof(uint8_t), meshData->MeshHeader.MeshBufferSizeInBytes, file);
    fclose(file);

    ElemCopyDataToGraphicsResourceParameters copyParameters =
    {
        .Resource = meshData->MeshBuffer.Buffer,
        .SourceType = ElemCopyDataSourceType_Memory,
        .SourceMemoryData = { .Items = (uint8_t*)meshBufferData, .Length = meshData->MeshHeader.MeshBufferSizeInBytes } 
    };

    ElemCopyDataToGraphicsResource(commandList, &copyParameters);
    free(meshBufferData);
}

void SampleFreeMesh(SampleMeshData* meshData)
{
    if(meshData->MeshBuffer.Buffer != ELEM_HANDLE_NULL)
    {
        SampleFreeGpuBuffer(&meshData->MeshBuffer);
    }

    // TODO: Free mallocs
}

void SampleLoadTexture(const char* path, SampleTextureData* textureData, SampleGpuMemory* gpuMemory)
{
    // TODO: Here we need to use a global list of texture that are unique based on the path
    // For now the same texture is loaded multiple time which is really bad

    *textureData = (SampleTextureData){};

    // TODO: Do better here
    char* tmp = malloc(strlen(path) + 1);
    strcpy(tmp, path);
    textureData->Path = tmp;

    FILE* file = SampleOpenFile(path, true);
    assert(file);
    
    fread(&textureData->TextureHeader, sizeof(SampleTextureHeader), 1, file);

    textureData->MipDataEntries = (SampleTextureDataBlockEntry*)malloc(sizeof(SampleTextureDataBlockEntry) * textureData->TextureHeader.MipCount);
    fread(textureData->MipDataEntries, sizeof(SampleTextureDataBlockEntry), textureData->TextureHeader.MipCount, file);

    fclose(file);

    // TODO: For now we create the texture here but later, we should do it on the fly and update the material buffer
    // TODO: Get texture name without folder and extension
    // TODO: Get the correct format
    textureData->GpuTexture = SampleCreateGpuTexture(gpuMemory, textureData->TextureHeader.Width, textureData->TextureHeader.Height, textureData->TextureHeader.MipCount, ElemGraphicsFormat_BC7_SRGB, textureData->Path);
}

// TODO: In the future we will need to load individual mips from disk
void SampleLoadTextureData(ElemCommandList commandList, SampleTextureData* textureData, SampleGpuMemory* gpuMemory)
{
    assert(textureData->Path);

    // TODO: Replace that with file logic
    for (uint32_t i = 0; i < textureData->TextureHeader.MipCount; i++)
    {
        SampleTextureDataBlockEntry mipEntry = textureData->MipDataEntries[i];
        uint8_t* mipData = (uint8_t*)malloc(sizeof(uint8_t) * mipEntry.SizeInBytes);

        FILE* file = SampleOpenFile(textureData->Path, true);
        assert(file);

        fseek(file, mipEntry.Offset, SEEK_SET);
        fread(mipData, sizeof(uint8_t), mipEntry.SizeInBytes, file);
        fclose(file);

        ElemCopyDataToGraphicsResourceParameters copyParameters =
        {
            .Resource = textureData->GpuTexture.Texture,
            .TextureMipLevel = i,
            .SourceType = ElemCopyDataSourceType_Memory,
            .SourceMemoryData = { .Items = (uint8_t*)mipData, .Length = mipEntry.SizeInBytes } 
        };

        ElemCopyDataToGraphicsResource(commandList, &copyParameters);
        free(mipData);
    }

    // TODO: This is not true, the data will be loaded when the command list is executed
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

void SampleLoadMaterial(const SampleSceneMaterialHeader* materialHeader, SampleMaterialData* materialData, ShaderMaterial* shaderMaterial, SampleGpuMemory* gpuMemory, const char* directoryPath)
{
    *materialData = (SampleMaterialData){};
    materialData->MaterialHeader = *materialHeader;

    shaderMaterial->AlbedoFactor = materialData->MaterialHeader.AlbedoFactor;
    shaderMaterial->AlbedoTextureId = -1;
    shaderMaterial->NormalTextureId = -1;

    // TODO: Some materials can use the same texture so we need to load it only once
    if (strlen(materialHeader->AlbedoTexturePath) > 0)
    {
        // TODO: Refactor that
        char fullTexturePath[MAX_PATH];
        memset(fullTexturePath, 0, MAX_PATH);
        strcpy(fullTexturePath, directoryPath);
        strcat(fullTexturePath, materialHeader->AlbedoTexturePath);

        SampleLoadTexture(fullTexturePath, &materialData->AlbedoTexture, gpuMemory);
        shaderMaterial->AlbedoTextureId = materialData->AlbedoTexture.GpuTexture.ReadDescriptor;
    }

    if (strlen(materialHeader->NormalTexturePath) > 0)
    {
        char fullTexturePath[MAX_PATH];
        memset(fullTexturePath, 0, MAX_PATH);
        strcpy(fullTexturePath, directoryPath);
        strcat(fullTexturePath, materialHeader->NormalTexturePath);

        SampleLoadTexture(fullTexturePath, &materialData->NormalTexture, gpuMemory);
        shaderMaterial->NormalTextureId = materialData->NormalTexture.GpuTexture.ReadDescriptor;
    }
}

void SampleFreeMaterial(SampleMaterialData* materialData)
{
    if (materialData->AlbedoTexture.GpuTexture.Texture)
    {
        SampleFreeTexture(&materialData->AlbedoTexture);
    }

    if (materialData->NormalTexture.GpuTexture.Texture)
    {
        SampleFreeTexture(&materialData->NormalTexture);
    }
}

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

    if (sceneHeader.MaterialCount > 0)
    {
        ShaderMaterial* shaderMaterials = (ShaderMaterial*)malloc(sizeof(ShaderMaterial) * sceneHeader.MaterialCount);

        for (uint32_t i = 0; i < sceneData->MaterialCount; i++)
        {
            SampleLoadMaterial(&materialHeaders[i], &sceneData->Materials[i], &shaderMaterials[i], gpuMemory, directoryPath);
        }

        sceneData->MaterialBuffer = SampleCreateGpuBufferAndUploadData(gpuMemory, shaderMaterials, sceneHeader.MaterialCount * sizeof(ShaderMaterial), "MaterialBuffer");
        free(shaderMaterials);
    }

    free(materialHeaders);
    free(meshDataBlocks);
}

void SampleFreeScene(SampleSceneData* sceneData)
{
    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleFreeMesh(&sceneData->Meshes[i]);
    }

    for (uint32_t i = 0; i < sceneData->MaterialCount; i++)
    {
        SampleFreeMaterial(&sceneData->Materials[i]);
    }

    if (sceneData->MaterialBuffer.Buffer != ELEM_HANDLE_NULL)
    {
        SampleFreeGpuBuffer(&sceneData->MaterialBuffer);
    }
}
