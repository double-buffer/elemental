#pragma once

#include "Elemental.h"
#include "SampleSceneLoader.h"
#include "SampleGpuMemory.h"
#include "SampleShaderData.h"

typedef struct
{
    bool IsLoaded;
    SampleGpuTexture* Textures;
    uint32_t TextureCount;
    SampleGpuBuffer* MeshBuffers;
    uint32_t MeshCount;
    SampleGpuBuffer MaterialBuffer;
    SampleGpuBuffer MeshInstanceBuffer;
    SampleGpuBuffer MeshPrimitiveInstanceBuffer;
    uint32_t MeshPrimitiveInstanceCount;
    uint32_t* MeshPrimitiveMeshletCountList;
} SampleGpuSceneData;

void SampleCreateGpuTextures(ElemCommandList commandList, const SampleSceneData* sceneData, SampleGpuSceneData* gpuSceneData, SampleGpuMemory* gpuMemory)
{
    gpuSceneData->Textures = (SampleGpuTexture*)malloc(sizeof(SampleGpuTexture) * sceneData->TextureCount);
    gpuSceneData->TextureCount = sceneData->TextureCount;

    for (uint32_t i = 0; i < sceneData->TextureCount; i++)
    {
        SampleTextureData* textureData = &sceneData->Textures[i];
        SampleGpuTexture* texture = &gpuSceneData->Textures[i];

        if (textureData->TextureHeader.Width == 0 || textureData->TextureHeader.Height == 0)
        {
            printf("No texture data!\n");
            continue;
        }

        ElemGraphicsFormat format = !textureData->IsNormalTexture ? ElemGraphicsFormat_BC7_SRGB : ElemGraphicsFormat_BC7;
        *texture = SampleCreateGpuTexture(gpuMemory, textureData->TextureHeader.Width, textureData->TextureHeader.Height, textureData->TextureHeader.MipCount, format, textureData->Path);

        for (uint32_t i = 0; i < textureData->TextureHeader.MipCount; i++)
        {
            SampleTextureDataBlockEntry mipEntry = textureData->MipDataEntries[i];
        
            if (strlen(textureData->Path) == 0)
            {
                continue;
            }

            char absolutePath[MAX_PATH];
            SampleGetFullPath(absolutePath, textureData->Path, true);

            ElemCopyDataToGraphicsResourceParameters copyParameters =
            {
                .Resource = texture->Texture,
                .TextureMipLevel = i,
                .SourceType = ElemCopyDataSourceType_File,
                .SourceFilePath = absolutePath,
                .SourceFileOffset = mipEntry.Offset,
                .SourceFileSizeInBytes = mipEntry.SizeInBytes
            };

            ElemCopyDataToGraphicsResource(commandList, &copyParameters);
        }
    }
}

void SampleCreateGpuMaterials(ElemCommandList commandList, const SampleSceneData* sceneData, SampleGpuSceneData* gpuSceneData, SampleGpuMemory* gpuMemory)
{
    ShaderMaterial* shaderMaterials = (ShaderMaterial*)malloc(sizeof(ShaderMaterial) * sceneData->MaterialCount);

    for (uint32_t i = 0; i < sceneData->MaterialCount; i++)
    {
        SampleSceneMaterialHeader* materialHeader = &sceneData->Materials[i];
        ShaderMaterial* shaderMaterial = &shaderMaterials[i];

        shaderMaterial->AlbedoFactor = materialHeader->AlbedoFactor;
        shaderMaterial->EmissiveFactor = materialHeader->EmissiveFactor;
        shaderMaterial->AlbedoTextureId = -1;
        shaderMaterial->NormalTextureId = -1;
        shaderMaterial->TransparentMode = materialHeader->TransparentMode;
        shaderMaterial->AlphaCutoff = materialHeader->AlphaCutoff;
        shaderMaterial->IsLoaded = true;

        if (gpuSceneData->TextureCount > 0)
        {
            if (materialHeader->AlbedoTextureId >= 0)
            {
                shaderMaterial->AlbedoTextureId = gpuSceneData->Textures[materialHeader->AlbedoTextureId].ReadDescriptor;
            }

            if (materialHeader->NormalTextureId >= 0)
            {
                shaderMaterial->NormalTextureId = gpuSceneData->Textures[materialHeader->NormalTextureId].ReadDescriptor;
            }
        }
    }

    gpuSceneData->MaterialBuffer = SampleCreateGpuBuffer(gpuMemory, sceneData->MaterialCount * sizeof(ShaderMaterial), ElemGraphicsResourceUsage_Read, "MaterialBuffer");

    ElemCopyDataToGraphicsResourceParameters copyParameters =
    {
        .Resource = gpuSceneData->MaterialBuffer.Buffer,
        .SourceType = ElemCopyDataSourceType_Memory,
        .SourceMemoryData = { .Items = (uint8_t*)shaderMaterials, .Length = sceneData->MaterialCount * sizeof(ShaderMaterial) } 
    };

    ElemCopyDataToGraphicsResource(commandList, &copyParameters);

    free(shaderMaterials);
}

void SampleCreateGpuMeshes(ElemCommandList commandList, const SampleSceneData* sceneData, SampleGpuSceneData* gpuSceneData, SampleGpuMemory* gpuMemory)
{
    gpuSceneData->MeshBuffers = (SampleGpuBuffer*)malloc(sizeof(SampleGpuBuffer) * sceneData->MeshCount);
    gpuSceneData->MeshCount = sceneData->MeshCount;

    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleMeshData* meshData = &sceneData->Meshes[i];

        SampleGpuBuffer* meshBuffer = &gpuSceneData->MeshBuffers[i];
        *meshBuffer = SampleCreateGpuBuffer(gpuMemory, meshData->MeshHeader.MeshBufferSizeInBytes, ElemGraphicsResourceUsage_Read, meshData->MeshHeader.Name);

        char absolutePath[MAX_PATH];
        SampleGetFullPath(absolutePath, meshData->Path, true);

        ElemCopyDataToGraphicsResourceParameters copyParameters =
        {
            .Resource = meshBuffer->Buffer,
            .SourceType = ElemCopyDataSourceType_File,
            .SourceFilePath = absolutePath,
            .SourceFileOffset = meshData->MeshHeader.MeshBufferOffset,
            .SourceFileSizeInBytes = meshData->MeshHeader.MeshBufferSizeInBytes
        };

        ElemCopyDataToGraphicsResource(commandList, &copyParameters);
    }
}

void SampleCreateGpuMeshInstances(ElemCommandList commandList, const SampleSceneData* sceneData, SampleGpuSceneData* gpuSceneData, SampleGpuMemory* gpuMemory)
{ 
    uint32_t gpuMeshInstanceCount = 0u;
    uint32_t gpuMeshPrimitiveInstanceCount = 0u;

    GpuMeshInstance* gpuMeshInstancesData = (GpuMeshInstance*)malloc(sizeof(GpuMeshInstance) * 10000);

    // TODO: Change the max value here
    GpuMeshPrimitiveInstance* gpuMeshPrimitiveInstancesData = (GpuMeshPrimitiveInstance*)malloc(sizeof(GpuMeshPrimitiveInstance) * 20000);
    uint32_t* gpuMeshPrimitiveInstancesMeshletCountList = (uint32_t*)malloc(sizeof(uint32_t) * 20000);

    for (uint32_t i = 0; i < sceneData->NodeCount; i++)
    {
        SampleSceneNodeHeader* sceneNode = &sceneData->Nodes[i];

        if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
        {
            GpuMeshInstance* gpuMeshInstance = &gpuMeshInstancesData[gpuMeshInstanceCount];
            SampleMeshData* meshData = &sceneData->Meshes[sceneNode->ReferenceIndex];

            gpuMeshInstance->Rotation = sceneNode->Rotation;
            gpuMeshInstance->Scale = sceneNode->Scale;
            gpuMeshInstance->Translation = sceneNode->Translation;
            gpuMeshInstance->MeshBufferIndex = gpuSceneData->MeshBuffers[sceneNode->ReferenceIndex].ReadDescriptor;

            for (uint32_t j = 0; j < meshData->MeshHeader.MeshPrimitiveCount; j++)
            {
                GpuMeshPrimitiveInstance* gpuMeshPrimitiveInstance = &gpuMeshPrimitiveInstancesData[gpuMeshPrimitiveInstanceCount];
                gpuMeshPrimitiveInstance->MeshInstanceId = gpuMeshInstanceCount;
                gpuMeshPrimitiveInstance->MeshPrimitiveId = j;

                gpuMeshPrimitiveInstancesMeshletCountList[gpuMeshPrimitiveInstanceCount] = meshData->MeshPrimitives[j].MeshletCount;
                gpuMeshPrimitiveInstanceCount++;
            }

            gpuMeshInstanceCount++;
        }
    }

    gpuSceneData->MeshInstanceBuffer = SampleCreateGpuBuffer(gpuMemory, gpuMeshInstanceCount * sizeof(GpuMeshInstance), ElemGraphicsResourceUsage_Read, "GpuMeshInstanceBuffer");

    ElemCopyDataToGraphicsResourceParameters copyParameters =
    {
        .Resource = gpuSceneData->MeshInstanceBuffer.Buffer,
        .SourceType = ElemCopyDataSourceType_Memory,
        .SourceMemoryData = { .Items = (uint8_t*)gpuMeshInstancesData, .Length = gpuMeshInstanceCount * sizeof(GpuMeshInstance) } 
    };

    ElemCopyDataToGraphicsResource(commandList, &copyParameters);

    gpuSceneData->MeshPrimitiveInstanceBuffer = SampleCreateGpuBuffer(gpuMemory, gpuMeshPrimitiveInstanceCount * sizeof(GpuMeshPrimitiveInstance), ElemGraphicsResourceUsage_Read, "GpuMeshPrimitiveInstanceBuffer");

    copyParameters = (ElemCopyDataToGraphicsResourceParameters)
    {
        .Resource = gpuSceneData->MeshPrimitiveInstanceBuffer.Buffer,
        .SourceType = ElemCopyDataSourceType_Memory,
        .SourceMemoryData = { .Items = (uint8_t*)gpuMeshPrimitiveInstancesData, .Length = gpuMeshPrimitiveInstanceCount * sizeof(GpuMeshPrimitiveInstance) } 
    };

    ElemCopyDataToGraphicsResource(commandList, &copyParameters);

    gpuSceneData->MeshPrimitiveInstanceCount = gpuMeshPrimitiveInstanceCount;
    gpuSceneData->MeshPrimitiveMeshletCountList = gpuMeshPrimitiveInstancesMeshletCountList;
    gpuSceneData->IsLoaded = true;

    free(gpuMeshInstancesData);
    free(gpuMeshPrimitiveInstancesData);

}

void SampleCreateGpuSceneData(ElemCommandList commandList, const SampleSceneData* sceneData, SampleGpuSceneData* gpuSceneData, SampleGpuMemory* gpuMemory)
{
    SampleCreateGpuTextures(commandList, sceneData, gpuSceneData, gpuMemory);
    SampleCreateGpuMaterials(commandList, sceneData, gpuSceneData, gpuMemory);
    SampleCreateGpuMeshes(commandList, sceneData, gpuSceneData, gpuMemory);
    SampleCreateGpuMeshInstances(commandList, sceneData, gpuSceneData, gpuMemory);
}

void SampleFreeGpuSceneData(SampleGpuSceneData* gpuSceneData)
{
    SampleFreeGpuBuffer(&gpuSceneData->MeshInstanceBuffer);
    SampleFreeGpuBuffer(&gpuSceneData->MeshPrimitiveInstanceBuffer);
    SampleFreeGpuBuffer(&gpuSceneData->MaterialBuffer);

    for (uint32_t i = 0; i < gpuSceneData->TextureCount; i++)
    {
        SampleFreeGpuTexture(&gpuSceneData->Textures[i]);
    }

    for (uint32_t i = 0; i < gpuSceneData->MeshCount; i++)
    {
        SampleFreeGpuBuffer(&gpuSceneData->MeshBuffers[i]);
    }
}
