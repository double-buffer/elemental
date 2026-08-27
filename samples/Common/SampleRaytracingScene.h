#pragma once

#include "Elemental.h"
#include "SampleSceneLoader.h"
#include "SampleGpuMemory.h"
#include "SampleGpuScene.h"

typedef struct
{
    uint64_t Offset;
    uint64_t SizeInBytes;
    uint64_t ScratchOffset;
    uint64_t ScratchSizeInBytes;
    ElemRaytracingBlasParameters BlasParameters;
    ElemGraphicsResource Blas;
} SampleRaytracingBlasData;

typedef struct
{
    SampleGpuBuffer BlasStorage;
    SampleGpuBuffer BlasScratchBuffer;
    SampleRaytracingBlasData* BlasData;
    uint32_t BlasCount;
    
    SampleGpuBuffer TlasInstanceBuffer;
    SampleGpuBuffer TlasStorage;
    SampleGpuBuffer TlasScratchBuffer;
    ElemGraphicsResource Tlas;
    ElemGraphicsResourceDescriptor TlasReadDescriptor;
} SampleRaytracingSceneData;

void SampleCreateRaytracingBlas(ElemGraphicsDevice graphicsDevice, ElemCommandList commandList, const SampleSceneData* sceneData, const SampleGpuSceneData* gpuSceneData, SampleRaytracingSceneData* raytracingSceneData, SampleGpuMemory* gpuMemory)
{
    raytracingSceneData->BlasCount = 0u;
    uint64_t currentBlasOffset = 0u;
    uint64_t currentBlasScratchOffset = 0u;

    raytracingSceneData->BlasData = (SampleRaytracingBlasData*)malloc(sceneData->MeshCount * sizeof(SampleRaytracingBlasData));

    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleMeshData* meshData = &sceneData->Meshes[i];
        SampleGpuBuffer meshBuffer = gpuSceneData->MeshBuffers[i];
        
        // TODO: Find another way
        ElemRaytracingBlasGeometry* geometry = (ElemRaytracingBlasGeometry*)malloc(meshData->MeshHeader.MeshPrimitiveCount * sizeof(ElemRaytracingBlasGeometry));

        for (uint32_t j = 0; j < meshData->MeshHeader.MeshPrimitiveCount; j++)
        {
            SampleMeshPrimitiveHeader* meshPrimitiveData = &meshData->MeshPrimitives[j];
            SampleSceneMaterialHeader* material = &sceneData->Materials[meshPrimitiveData->MaterialId];

            geometry[j] = (ElemRaytracingBlasGeometry)
            {
                .VertexFormat = ElemRaytracingVertexFormat_Float32,
                .VertexBuffer = meshBuffer.Buffer,
                .VertexBufferOffset = meshPrimitiveData->VertexBufferOffset,
                .VertexCount = meshPrimitiveData->VertexCount,
                .VertexSizeInBytes = meshData->MeshHeader.VertexSizeInBytes,
                .IndexFormat = ElemRaytracingIndexFormat_UInt32,
                .IndexBuffer = meshBuffer.Buffer,
                .IndexBufferOffset = meshPrimitiveData->IndexBufferOffset,
                .IndexCount = meshPrimitiveData->IndexCount,
                .IsTransparent = material->TransparentMode != SampleSceneMaterialTransparentMode_None
            };
        }

        ElemRaytracingBlasParameters blasParameters =
        {
            .BuildFlags = ElemRaytracingBuildFlags_PreferFastTrace,
            .GeometryList = { .Items = geometry, .Length = meshData->MeshHeader.MeshPrimitiveCount }
        };

        ElemRaytracingAllocationInfo allocationInfos = ElemGetRaytracingBlasAllocationInfo(graphicsDevice, &blasParameters);

        raytracingSceneData->BlasData[raytracingSceneData->BlasCount++] = (SampleRaytracingBlasData)
        { 
            .Offset = currentBlasOffset, 
            .SizeInBytes = allocationInfos.SizeInBytes,
            .ScratchOffset = currentBlasScratchOffset,
            .ScratchSizeInBytes = allocationInfos.ScratchSizeInBytes,
            .BlasParameters = blasParameters
        };

        currentBlasOffset = SampleAlignValue(currentBlasOffset + allocationInfos.SizeInBytes, allocationInfos.Alignment);
        currentBlasScratchOffset = SampleAlignValue(currentBlasScratchOffset + allocationInfos.ScratchSizeInBytes, allocationInfos.Alignment);
    }

    raytracingSceneData->BlasScratchBuffer = SampleCreateGpuBuffer(gpuMemory, currentBlasScratchOffset, ElemGraphicsResourceUsage_Write, "GlobalBlasScratch");
    raytracingSceneData->BlasStorage = SampleCreateGpuBuffer(gpuMemory, currentBlasOffset, ElemGraphicsResourceUsage_RaytracingAccelerationStructure, "GlobalBlasStorage");
    
    char formattedSize[256];
    FormatMemorySize((uint32_t)currentBlasOffset, formattedSize, 256);
    printf("BLAS Size: %s\n", formattedSize);

    // TODO: Write compact acceleration structure code

    for (uint32_t i = 0; i < raytracingSceneData->BlasCount; i++)
    {
        SampleRaytracingBlasData* blasInfo = &raytracingSceneData->BlasData[i];

        blasInfo->Blas = ElemCreateRaytracingAccelerationStructureResource(graphicsDevice, 
                                                                         raytracingSceneData->BlasStorage.Buffer, 
                                                                         &(ElemRaytracingAccelerationStructureOptions)
                                                                         {
                                                                            .StorageOffset = blasInfo->Offset,
                                                                            .StorageSizeInBytes = blasInfo->SizeInBytes
                                                                         });
    }

    // TODO: Do we really need this?
    ElemGraphicsResourceBarrier(commandList, raytracingSceneData->BlasStorage.WriteDescriptor, NULL);
    ElemGraphicsResourceBarrier(commandList, raytracingSceneData->BlasScratchBuffer.WriteDescriptor, NULL);

    for (uint32_t i = 0; i < raytracingSceneData->BlasCount; i++)
    {
        SampleRaytracingBlasData* blasInfo = &raytracingSceneData->BlasData[i];

        ElemBuildRaytracingBlas(commandList, blasInfo->Blas, 
                                             raytracingSceneData->BlasScratchBuffer.Buffer, 
                                             &blasInfo->BlasParameters, 
                                             &(ElemRaytracingBuildOptions) { .ScratchOffset = blasInfo->ScratchOffset });
    }
        
    ElemGraphicsResourceBarrier(commandList, raytracingSceneData->BlasStorage.ReadDescriptor, NULL);
}

void SampleCreateRaytracingTlas(ElemGraphicsDevice graphicsDevice, ElemCommandList commandList, const SampleSceneData* sceneData, const SampleGpuSceneData* gpuSceneData, SampleRaytracingSceneData* raytracingSceneData, SampleGpuMemory* gpuMemory, SampleGpuMemory* gpuMemoryUpload)
{
    uint32_t tlasInstanceCount = 0u;

    for (uint32_t i = 0; i < sceneData->NodeCount; i++)
    {
        SampleSceneNodeHeader* sceneNode = &sceneData->Nodes[i];

        if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
        {
            tlasInstanceCount++;
        }
    }
    
    ElemGraphicsResourceAllocationInfo tlasInstanceAllocationInfo = ElemGetRaytracingTlasInstanceAllocationInfo(graphicsDevice, tlasInstanceCount);
    raytracingSceneData->TlasInstanceBuffer = SampleCreateGpuBuffer(gpuMemoryUpload, tlasInstanceAllocationInfo.SizeInBytes, ElemGraphicsResourceUsage_Write, "TlasInstanceBuffer");

    ElemRaytracingTlasParameters tlasParameters =
    {
        .BuildFlags = ElemRaytracingBuildFlags_PreferFastTrace,
        .InstanceCount = tlasInstanceCount,
    };

    ElemRaytracingAllocationInfo allocationInfos = ElemGetRaytracingTlasAllocationInfo(graphicsDevice, &tlasParameters);

    raytracingSceneData->TlasStorage = SampleCreateGpuBuffer(gpuMemory, allocationInfos.SizeInBytes, ElemGraphicsResourceUsage_RaytracingAccelerationStructure, "TLASAccelStorage");
    raytracingSceneData->TlasScratchBuffer = SampleCreateGpuBuffer(gpuMemory, allocationInfos.ScratchSizeInBytes, ElemGraphicsResourceUsage_Write, "TLASScratchStorage");
    raytracingSceneData->Tlas = ElemCreateRaytracingAccelerationStructureResource(graphicsDevice, raytracingSceneData->TlasStorage.Buffer, NULL);
    raytracingSceneData->TlasReadDescriptor = ElemCreateGraphicsResourceDescriptor(raytracingSceneData->Tlas, ElemGraphicsResourceDescriptorUsage_Read, NULL);
}

void SampleBuildRaytracingTlas(ElemCommandList commandList, const SampleSceneData* sceneData, SampleRaytracingSceneData* raytracingSceneData)
{
    // TODO: Move that part in the other function
    ElemRaytracingTlasInstance tlasInstances[10000];
    uint32_t tlasInstanceCount = 0u;

    // TODO: Reconstructing the whole instance buffer is bad, we should only update what was changed and update the sysmem part
    for (uint32_t i = 0; i < sceneData->NodeCount; i++)
    {
        SampleSceneNodeHeader* sceneNode = &sceneData->Nodes[i];

        if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
        {
            SampleRaytracingBlasData* blasData = &raytracingSceneData->BlasData[sceneNode->ReferenceIndex];

            ElemMatrix4x3 transformMatrix = SampleCreateTransformMatrix2(sceneNode->Rotation, sceneNode->Scale, sceneNode->Translation);

            tlasInstances[tlasInstanceCount] = (ElemRaytracingTlasInstance)
            {
                .InstanceId = tlasInstanceCount,
                .InstanceMask = 1,
                .TransformMatrix = transformMatrix,
                .BlasResource = blasData->Blas
            };

            tlasInstanceCount++;
        }
    }

    // TODO: Maybe we could have a system instead to return an array of span so that we can update the instances we need?
    ElemDataSpan tlasInstanceData = ElemEncodeRaytracingTlasInstances((ElemRaytracingTlasInstanceSpan) { .Items = tlasInstances, .Length = tlasInstanceCount });
    ElemUploadGraphicsBufferData(raytracingSceneData->TlasInstanceBuffer.Buffer, 0, tlasInstanceData);

    ElemRaytracingTlasParameters tlasParameters =
    {
        .BuildFlags = ElemRaytracingBuildFlags_PreferFastTrace,
        .InstanceBuffer = raytracingSceneData->TlasInstanceBuffer.Buffer,
        .InstanceCount = tlasInstanceCount,
    };

    ElemBuildRaytracingTlas(commandList, raytracingSceneData->Tlas, raytracingSceneData->TlasScratchBuffer.Buffer, &tlasParameters, NULL);
}

void SampleCreateRaytracingSceneData(ElemGraphicsDevice graphicsDevice, ElemCommandList commandList, const SampleSceneData* sceneData, const SampleGpuSceneData* gpuSceneData, SampleRaytracingSceneData* raytracingSceneData, SampleGpuMemory* gpuMemory, SampleGpuMemory* gpuMemoryUpload)
{
    SampleCreateRaytracingBlas(graphicsDevice, commandList, sceneData, gpuSceneData, raytracingSceneData, gpuMemory);
    SampleCreateRaytracingTlas(graphicsDevice, commandList, sceneData, gpuSceneData, raytracingSceneData, gpuMemory, gpuMemoryUpload);
    SampleBuildRaytracingTlas(commandList, sceneData, raytracingSceneData);
}

void SampleFreeRaytracingSceneData(SampleRaytracingSceneData* raytracingSceneData)
{
    SampleFreeGpuBuffer(&raytracingSceneData->BlasStorage);
    SampleFreeGpuBuffer(&raytracingSceneData->BlasScratchBuffer);

    for (uint32_t i = 0; i < raytracingSceneData->BlasCount; i++)
    {
        ElemFreeGraphicsResource(raytracingSceneData->BlasData[i].Blas, NULL);
    }

    ElemFreeGraphicsResourceDescriptor(raytracingSceneData->TlasReadDescriptor, NULL);
    ElemFreeGraphicsResource(raytracingSceneData->Tlas, NULL);
    SampleFreeGpuBuffer(&raytracingSceneData->TlasInstanceBuffer);
    SampleFreeGpuBuffer(&raytracingSceneData->TlasStorage);
    SampleFreeGpuBuffer(&raytracingSceneData->TlasScratchBuffer);

    free(raytracingSceneData->BlasData);
}
