#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"
#include "SampleInputsApplication.h"
#include "SampleInputsCamera.h"
#include "SampleSceneLoader.h"

// TODO: Move the gpu buffers creation from sceneloading to main
// TODO: Remove Rasterization path
// TODO: Remove the depth buffer
// TODO: Change the shortcut for animation to space
// TODO: Refactor Raytracing shader
// TODO: Use a compute shader for raytracing?

typedef struct
{
    uint32_t ShaderGlobalParametersBuffer;
    uint32_t MeshPrimitiveInstanceId;
} ShaderParameters;

typedef struct
{
    uint32_t AccelerationStructureIndex;
    uint32_t ShaderGlobalParametersBufferIndex;
    uint32_t FrameIndex;
    uint32_t PathTraceLength;
} RaytracingShaderParameters;

typedef struct
{
    uint32_t SourceTexture;
    uint32_t SampleCount;
} ToneMapShaderParameters;

typedef struct
{
    SampleMatrix4x4 ViewProjMatrix;
    SampleMatrix4x4 InverseViewMatrix;
    SampleMatrix4x4 InverseProjectionMatrix;
    uint32_t MaterialBufferIndex;
    uint32_t GpuMeshInstanceBufferIndex;
    uint32_t GpuMeshPrimitiveInstanceBufferIndex;
    uint32_t Action;
} ShaderShaderGlobalParameters;

typedef struct
{
    uint64_t Offset;
    uint64_t SizeInBytes;
    uint64_t ScratchOffset;
    uint64_t ScratchSizeInBytes;
    ElemRaytracingBlasParameters BlasParameters;
    ElemGraphicsResource Blas;
} RaytracingBlasData;

// TODO: Group common variables into separate structs
typedef struct
{
    SampleAppSettings AppSettings;
    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    uint32_t CurrentHeapOffset;
    ElemFence LastExecutionFence;
    ElemSwapChain SwapChain;
    ElemGraphicsHeap DepthBufferHeap;
    ElemGraphicsHeap RenderTargetHeap;
    ElemGraphicsResource DepthBuffer;
    ElemGraphicsResource RenderTargetTexture;
    ElemGraphicsResourceDescriptor RenderTargetTextureReadDescriptor;
    ElemPipelineState GraphicsPipeline;
    ElemPipelineState ToneMapGraphicsPipeline;
    ElemPipelineState RaytracingGraphicsPipeline;
    ShaderParameters ShaderParameters;
    SampleInputsApplication InputsApplication;
    SampleInputsCamera InputsCamera;
    SampleSceneData TestSceneData; // TODO: Get rid of scene data after loading?
    SampleGpuMemory GpuMemory;
    SampleGpuMemory GpuMemoryUpload;

    ShaderShaderGlobalParameters ShaderGlobalParameters;
    SampleGpuBuffer ShaderGlobalParametersBuffer;

    SampleGpuBuffer GpuMeshInstanceBuffer;
    SampleGpuBuffer GpuMeshPrimitiveInstanceBuffer;
    uint32_t GpuMeshPrimitiveInstanceCount;
    uint32_t* GpuMeshPrimitiveMeshletCountList;

    uint32_t PathTracingSamplingCount;
    uint32_t PathTraceLength;
    bool UsePathTracing;
    bool UsePathTracingAccumulation;
    SampleGpuBuffer BlasStorage;
    SampleGpuBuffer BlasScratchBuffer;
    RaytracingBlasData* BlasData;
    uint32_t BlasCount;
    
    SampleGpuBuffer TlasInstanceBuffer;
    SampleGpuBuffer TlasStorage;
    SampleGpuBuffer TlasScratchBuffer;
    ElemGraphicsResource Tlas;
    ElemGraphicsResourceDescriptor TlasReadDescriptor;

    bool UseAnimation;
    float AnimationDirection;
} ApplicationPayload;
    
typedef struct
{
    SampleInputsCameraState CameraState; 
} SavedState;

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

void CreateDepthBuffer(ApplicationPayload* applicationPayload, uint32_t width, uint32_t height)
{
    if (applicationPayload->DepthBuffer != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    }

    printf("Creating DepthBuffer...\n");

    ElemGraphicsResourceInfo resourceInfo = ElemCreateTexture2DResourceInfo(applicationPayload->GraphicsDevice, width, height, 1, ElemGraphicsFormat_D32_FLOAT, ElemGraphicsResourceUsage_DepthStencil,
                                                                            &(ElemGraphicsResourceInfoOptions) { 
                                                                                .DebugName = "DepthBuffer" 
                                                                            });

    applicationPayload->DepthBuffer = ElemCreateGraphicsResource(applicationPayload->DepthBufferHeap, 0, &resourceInfo);
}

void CreateRenderTarget(ApplicationPayload* applicationPayload, uint32_t width, uint32_t height)
{
    if (applicationPayload->RenderTargetTexture != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsResourceDescriptor(applicationPayload->RenderTargetTextureReadDescriptor, NULL);
        ElemFreeGraphicsResource(applicationPayload->RenderTargetTexture, NULL);
    }

    printf("Creating render texture...\n");

    ElemGraphicsResourceInfo resourceInfo = ElemCreateTexture2DResourceInfo(applicationPayload->GraphicsDevice, width, height, 1, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget,
                                                                            &(ElemGraphicsResourceInfoOptions) { 
                                                                                .DebugName = "FloatRenderTarget" 
                                                                            });

    applicationPayload->RenderTargetTexture = ElemCreateGraphicsResource(applicationPayload->RenderTargetHeap, 0, &resourceInfo);
    applicationPayload->RenderTargetTextureReadDescriptor = ElemCreateGraphicsResourceDescriptor(applicationPayload->RenderTargetTexture, ElemGraphicsResourceDescriptorUsage_Read, NULL);
}

void InitSceneGpuBuffers(ApplicationPayload* applicationPayload)
{
    SampleSceneData* sceneData = &applicationPayload->TestSceneData;

    GpuMeshInstance* gpuMeshInstancesData = (GpuMeshInstance*)malloc(sizeof(GpuMeshInstance) * 10000);
    uint32_t gpuMeshInstanceCount = 0u;

    // TODO: Change the max value here
    GpuMeshPrimitiveInstance* gpuMeshPrimitiveInstancesData = (GpuMeshPrimitiveInstance*)malloc(sizeof(GpuMeshPrimitiveInstance) * 20000);
    uint32_t* gpuMeshPrimitiveInstancesMeshletCountList = (uint32_t*)malloc(sizeof(uint32_t) * 20000);
    uint32_t gpuMeshPrimitiveInstanceCount = 0u;

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
            //gpuMeshInstance->MeshBufferIndex = meshData->MeshBuffer.ReadDescriptor;

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

    applicationPayload->GpuMeshInstanceBuffer = SampleCreateGpuBufferAndUploadData(&applicationPayload->GpuMemoryUpload, gpuMeshInstancesData, gpuMeshInstanceCount * sizeof(GpuMeshInstance), "GpuMeshInstanceBuffer");
    applicationPayload->GpuMeshPrimitiveInstanceBuffer = SampleCreateGpuBufferAndUploadData(&applicationPayload->GpuMemoryUpload, gpuMeshPrimitiveInstancesData, gpuMeshPrimitiveInstanceCount * sizeof(GpuMeshPrimitiveInstance), "GpuMeshPrimitiveInstanceBuffer");
    applicationPayload->GpuMeshPrimitiveInstanceCount = gpuMeshPrimitiveInstanceCount;
    applicationPayload->GpuMeshPrimitiveMeshletCountList = gpuMeshPrimitiveInstancesMeshletCountList;

    free(gpuMeshInstancesData);
}

void CreateRaytracingBlas(ElemCommandList commandList, ApplicationPayload* applicationPayload)
{
    SampleSceneData* sceneData = &applicationPayload->TestSceneData;

    applicationPayload->BlasCount = 0u;
    uint64_t currentBlasOffset = 0u;
    uint64_t currentBlasScratchOffset = 0u;

    applicationPayload->BlasData = (RaytracingBlasData*)malloc(sceneData->MeshCount * sizeof(RaytracingBlasData));

    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleMeshData* meshData = &sceneData->Meshes[i];
        
        // TODO: Find another way
        ElemRaytracingBlasGeometry* geometry = (ElemRaytracingBlasGeometry*)malloc(meshData->MeshHeader.MeshPrimitiveCount * sizeof(ElemRaytracingBlasGeometry));

        for (uint32_t j = 0; j < meshData->MeshHeader.MeshPrimitiveCount; j++)
        {
            SampleMeshPrimitiveHeader* meshPrimitiveData = &meshData->MeshPrimitives[j];

            geometry[j] = (ElemRaytracingBlasGeometry)
            {
                .VertexFormat = ElemRaytracingVertexFormat_Float32,
                //.VertexBuffer = meshData->MeshBuffer.Buffer,
                .VertexBufferOffset = meshPrimitiveData->VertexBufferOffset,
                .VertexCount = meshPrimitiveData->VertexCount,
                .VertexSizeInBytes = meshData->MeshHeader.VertexSizeInBytes,
                .IndexFormat = ElemRaytracingIndexFormat_UInt32,
                //.IndexBuffer = meshData->MeshBuffer.Buffer,
                .IndexBufferOffset = meshPrimitiveData->IndexBufferOffset,
                .IndexCount = meshPrimitiveData->IndexCount
            };
        }

        ElemRaytracingBlasParameters blasParameters =
        {
            .BuildFlags = ElemRaytracingBuildFlags_PreferFastTrace,
            .GeometryList = { .Items = geometry, .Length = meshData->MeshHeader.MeshPrimitiveCount }
        };

        ElemRaytracingAllocationInfo allocationInfos = ElemGetRaytracingBlasAllocationInfo(applicationPayload->GraphicsDevice, &blasParameters);

        applicationPayload->BlasData[applicationPayload->BlasCount++] = (RaytracingBlasData)
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

    applicationPayload->BlasScratchBuffer = SampleCreateGpuBuffer(&applicationPayload->GpuMemory, currentBlasScratchOffset, "GlobalBlasScratch");
    applicationPayload->BlasStorage = SampleCreateGpuRaytracingBuffer(&applicationPayload->GpuMemory, currentBlasOffset, "GlobalBlasStorage");
    
    char formattedSize[256];
    FormatMemorySize((uint32_t)currentBlasOffset, formattedSize, 256);
    printf("BLAS Size: %s\n", formattedSize);

    // TODO: Write compact acceleration structure code

    for (uint32_t i = 0; i < applicationPayload->BlasCount; i++)
    {
        RaytracingBlasData* blasInfo = &applicationPayload->BlasData[i];

        blasInfo->Blas = ElemCreateRaytracingAccelerationStructureResource(applicationPayload->GraphicsDevice, 
                                                                         applicationPayload->BlasStorage.Buffer, 
                                                                         &(ElemRaytracingAccelerationStructureOptions)
                                                                         {
                                                                            .StorageOffset = blasInfo->Offset,
                                                                            .StorageSizeInBytes = blasInfo->SizeInBytes
                                                                         });
    }

    ElemGraphicsResourceBarrier(commandList, applicationPayload->BlasStorage.WriteDescriptor, NULL);

    for (uint32_t i = 0; i < applicationPayload->BlasCount; i++)
    {
        RaytracingBlasData* blasInfo = &applicationPayload->BlasData[i];

        ElemBuildRaytracingBlas(commandList, blasInfo->Blas, 
                                             applicationPayload->BlasScratchBuffer.Buffer, 
                                             &blasInfo->BlasParameters, 
                                             &(ElemRaytracingBuildOptions) { .ScratchOffset = blasInfo->ScratchOffset });
    }
        
    ElemGraphicsResourceBarrier(commandList, applicationPayload->BlasStorage.ReadDescriptor, NULL);
}

void CreateRaytracingTlas(ApplicationPayload* applicationPayload)
{
    SampleSceneData* sceneData = &applicationPayload->TestSceneData;

    uint32_t tlasInstanceCount = 0u;

    for (uint32_t i = 0; i < sceneData->NodeCount; i++)
    {
        SampleSceneNodeHeader* sceneNode = &sceneData->Nodes[i];

        if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
        {
            tlasInstanceCount++;
        }
    }
    
    ElemGraphicsResourceAllocationInfo tlasInstanceAllocationInfo = ElemGetRaytracingTlasInstanceAllocationInfo(applicationPayload->GraphicsDevice, tlasInstanceCount);
    applicationPayload->TlasInstanceBuffer = SampleCreateGpuBuffer(&applicationPayload->GpuMemoryUpload, tlasInstanceAllocationInfo.SizeInBytes, "TlasInstanceBuffer");

    ElemRaytracingTlasParameters tlasParameters =
    {
        .BuildFlags = ElemRaytracingBuildFlags_PreferFastTrace,
        .InstanceCount = tlasInstanceCount,
    };

    ElemRaytracingAllocationInfo allocationInfos = ElemGetRaytracingTlasAllocationInfo(applicationPayload->GraphicsDevice, &tlasParameters);

    applicationPayload->TlasStorage = SampleCreateGpuRaytracingBuffer(&applicationPayload->GpuMemory, allocationInfos.SizeInBytes, "TLASAccelStorage");
    applicationPayload->TlasScratchBuffer = SampleCreateGpuBuffer(&applicationPayload->GpuMemory, allocationInfos.ScratchSizeInBytes, "TLASScratchStorage");
    applicationPayload->Tlas = ElemCreateRaytracingAccelerationStructureResource(applicationPayload->GraphicsDevice, applicationPayload->TlasStorage.Buffer, NULL);
    applicationPayload->TlasReadDescriptor = ElemCreateGraphicsResourceDescriptor(applicationPayload->Tlas, ElemGraphicsResourceDescriptorUsage_Read, NULL);
}

void BuildRaytracingTlas(ElemCommandList commandList, ApplicationPayload* applicationPayload)
{
    SampleSceneData* sceneData = &applicationPayload->TestSceneData;

    // TODO: Move that part in the other function
    ElemRaytracingTlasInstance tlasInstances[1024];
    uint32_t tlasInstanceCount = 0u;

    for (uint32_t i = 0; i < sceneData->NodeCount; i++)
    {
        SampleSceneNodeHeader* sceneNode = &sceneData->Nodes[i];

        if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
        {
            RaytracingBlasData* blasData = &applicationPayload->BlasData[sceneNode->ReferenceIndex];

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

    ElemDataSpan tlasInstanceData = ElemEncodeRaytracingTlasInstances((ElemRaytracingTlasInstanceSpan) { .Items = tlasInstances, .Length = tlasInstanceCount });
    ElemUploadGraphicsBufferData(applicationPayload->TlasInstanceBuffer.Buffer, 0, tlasInstanceData);

    ElemRaytracingTlasParameters tlasParameters =
    {
        .BuildFlags = ElemRaytracingBuildFlags_PreferFastTrace,
        .InstanceBuffer = applicationPayload->TlasInstanceBuffer.Buffer,
        .InstanceCount = tlasInstanceCount,
    };

    ElemBuildRaytracingTlas(commandList, applicationPayload->Tlas, applicationPayload->TlasScratchBuffer.Buffer, &tlasParameters, NULL);
}

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->Window = ElemCreateWindow(&(ElemWindowOptions) { .WindowState = applicationPayload->AppSettings.PreferFullScreen ? ElemWindowState_FullScreen : ElemWindowState_Normal });

    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = !applicationPayload->AppSettings.DisableDiagnostics, .EnableGpuValidation = false, .EnableDebugBarrierInfo = false, .PreferVulkan = applicationPayload->AppSettings.PreferVulkan });
    
    applicationPayload->GraphicsDevice = ElemCreateGraphicsDevice(NULL);

    applicationPayload->CommandQueue= ElemCreateCommandQueue(applicationPayload->GraphicsDevice, ElemCommandQueueType_Graphics, NULL);
    applicationPayload->SwapChain= ElemCreateSwapChain(applicationPayload->CommandQueue, applicationPayload->Window, UpdateSwapChain, &(ElemSwapChainOptions) { 
        .FrameLatency = 1, 
        .UpdatePayload = payload, 
        .Format = ElemSwapChainFormat_Default 
    });
    
    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(applicationPayload->SwapChain);

    // TODO: For now we create a separate heap to avoid memory management
    applicationPayload->DepthBufferHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_Gpu });
    applicationPayload->RenderTargetHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(128), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_Gpu });
    applicationPayload->GpuMemory = SampleCreateGpuMemory(applicationPayload->GraphicsDevice, ElemGraphicsHeapType_Gpu, SampleMegaBytesToBytes(256));
    applicationPayload->GpuMemoryUpload = SampleCreateGpuMemory(applicationPayload->GraphicsDevice, ElemGraphicsHeapType_GpuUpload, SampleMegaBytesToBytes(256));

    CreateDepthBuffer(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);
    CreateRenderTarget(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);
    SampleLoadScene("CornellBox.scene", &applicationPayload->TestSceneData, &applicationPayload->GpuMemoryUpload);
    //SampleLoadScene("sponza.scene", &applicationPayload->TestSceneData, &applicationPayload->GpuMemoryUpload);
    InitSceneGpuBuffers(applicationPayload);
    
    applicationPayload->ShaderGlobalParametersBuffer = SampleCreateGpuBufferAndUploadData(&applicationPayload->GpuMemoryUpload, &applicationPayload->ShaderGlobalParameters, sizeof(ShaderShaderGlobalParameters), "ShaderGlobalParameters");
    applicationPayload->ShaderParameters.ShaderGlobalParametersBuffer = applicationPayload->ShaderGlobalParametersBuffer.ReadDescriptor;
    applicationPayload->PathTraceLength = 3;
    applicationPayload->UsePathTracing = true;
    applicationPayload->UsePathTracingAccumulation = true;
    applicationPayload->AnimationDirection = 1;

    ElemCommandList loadDataCommandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL);
    
    for (uint32_t i = 0; i < applicationPayload->TestSceneData.NodeCount; i++)
    {
        SampleSceneNodeHeader* sceneNode = &applicationPayload->TestSceneData.Nodes[i];

        if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
        {
            SampleMeshData* meshData = &applicationPayload->TestSceneData.Meshes[sceneNode->ReferenceIndex];
            //SampleLoadMeshData(loadDataCommandList, meshData, &applicationPayload->GpuMemoryUpload);
        }
    }

    CreateRaytracingBlas(loadDataCommandList, applicationPayload);
    CreateRaytracingTlas(applicationPayload);
    BuildRaytracingTlas(loadDataCommandList, applicationPayload);

    ElemCommitCommandList(loadDataCommandList);
    ElemFence loadFence = ElemExecuteCommandList(applicationPayload->CommandQueue, loadDataCommandList, NULL);

    SampleFreeGpuBufferWithFence(&applicationPayload->BlasScratchBuffer, loadFence);

    ElemDataSpan shaderData = SampleReadFile(!applicationPayload->AppSettings.PreferVulkan ? "RenderMesh.shader": "RenderMesh_vulkan.shader", true);
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, shaderData);

    applicationPayload->GraphicsPipeline = ElemCompileGraphicsPipelineState(applicationPayload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "RenderMesh PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {{ .Format = ElemGraphicsFormat_R32G32B32A32_FLOAT }}, .Length = 1 },
        .DepthStencil =
        {
            .Format = ElemGraphicsFormat_D32_FLOAT,
            .DepthCompareFunction = ElemGraphicsCompareFunction_Greater
        }
    });

    ElemFreeShaderLibrary(shaderLibrary);

    shaderData = SampleReadFile(!applicationPayload->AppSettings.PreferVulkan ? "Raytracing.shader": "Raytracing_vulkan.shader", true);
    shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, shaderData);

    applicationPayload->RaytracingGraphicsPipeline = ElemCompileGraphicsPipelineState(applicationPayload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "RayTracing PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {
        { 
            .Format = ElemGraphicsFormat_R32G32B32A32_FLOAT,
            .BlendOperation = ElemGraphicsBlendOperation_Add,
            .SourceBlendFactor = ElemGraphicsBlendFactor_One,
            .DestinationBlendFactor = ElemGraphicsBlendFactor_One,
            .SourceBlendFactorAlpha = ElemGraphicsBlendFactor_One,
            .DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_One,
        }}, .Length = 1 },
    });

    ElemFreeShaderLibrary(shaderLibrary);
    
    shaderData = SampleReadFile(!applicationPayload->AppSettings.PreferVulkan ? "Tonemap.shader": "Tonemap_vulkan.shader", true);
    shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, shaderData);

    applicationPayload->ToneMapGraphicsPipeline = ElemCompileGraphicsPipelineState(applicationPayload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "Tonemap PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {
        { 
            .Format = swapChainInfo.Format,
            .BlendOperation = ElemGraphicsBlendOperation_Add,
            .SourceBlendFactor = ElemGraphicsBlendFactor_SourceAlpha,
            .DestinationBlendFactor = ElemGraphicsBlendFactor_InverseSourceAlpha,
        }}, .Length = 1 },
    });

    ElemFreeShaderLibrary(shaderLibrary);

    SampleInputsApplicationInit(&applicationPayload->InputsApplication);
    SampleInputsCameraInit(&applicationPayload->InputsCamera);

    applicationPayload->InputsCamera.State.Camera.Position = (ElemVector3) { 0.0f, 1.0f, -3.4f };
    applicationPayload->InputsCamera.State.Camera.Rotation = (ElemVector3) { 0.0f, 0.0f, 0 };
    
    if (applicationPayload->AppSettings.PreferFullScreen)
    {
        ElemHideWindowCursor(applicationPayload->Window);
        applicationPayload->InputsApplication.State.IsCursorDisplayed = false;
    }

    SampleStartFrameMeasurement();

    ElemDataSpan savedStateData = SampleReadFile("SavedState.bin", false);

    if (savedStateData.Length > 0)
    {
        applicationPayload->InputsCamera.State = ((SavedState*)savedStateData.Items)->CameraState;
        applicationPayload->InputsCamera.State.ProjectionMatrix = (SampleMatrix4x4){};
    }
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemWaitForFenceOnCpu(applicationPayload->LastExecutionFence);

    SampleFreeScene(&applicationPayload->TestSceneData);
    SampleFreeGpuBuffer(&applicationPayload->ShaderGlobalParametersBuffer);

    SampleFreeGpuBuffer(&applicationPayload->BlasStorage);

    for (uint32_t i = 0; i < applicationPayload->BlasCount; i++)
    {
        ElemFreeGraphicsResource(applicationPayload->BlasData[i].Blas, NULL);
    }

    ElemFreeGraphicsResourceDescriptor(applicationPayload->TlasReadDescriptor, NULL);
    ElemFreeGraphicsResource(applicationPayload->Tlas, NULL);
    SampleFreeGpuBuffer(&applicationPayload->TlasInstanceBuffer);
    SampleFreeGpuBuffer(&applicationPayload->TlasStorage);
    SampleFreeGpuBuffer(&applicationPayload->TlasScratchBuffer);

    free(applicationPayload->BlasData);

    ElemFreePipelineState(applicationPayload->ToneMapGraphicsPipeline);
    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreePipelineState(applicationPayload->RaytracingGraphicsPipeline);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
 
    ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    ElemFreeGraphicsResource(applicationPayload->RenderTargetTexture, NULL);
    ElemFreeGraphicsHeap(applicationPayload->DepthBufferHeap);
    ElemFreeGraphicsHeap(applicationPayload->RenderTargetHeap);

    SampleFreeGpuMemory(&applicationPayload->GpuMemoryUpload);
    SampleFreeGpuMemory(&applicationPayload->GpuMemory);

    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);
    
    SavedState savedState = { .CameraState = applicationPayload->InputsCamera.State }; 
    SampleWriteDataToApplicationFile("SavedState.bin", (ElemDataSpan) { .Items = (uint8_t*)&savedState, .Length = sizeof(SavedState) }, false);

    printf("Exit application...\n");
}

void UpdateShaderGlobalParameters(ApplicationPayload* applicationPayload, const SampleInputsCameraState* cameraState)
{
    applicationPayload->ShaderGlobalParameters.ViewProjMatrix = cameraState->ViewProjMatrix;
    applicationPayload->ShaderGlobalParameters.InverseViewMatrix = cameraState->InverseViewMatrix;
    applicationPayload->ShaderGlobalParameters.InverseProjectionMatrix = cameraState->InverseProjectionMatrix;
    //applicationPayload->ShaderGlobalParameters.MaterialBufferIndex = applicationPayload->TestSceneData.MaterialBuffer.ReadDescriptor;
    applicationPayload->ShaderGlobalParameters.GpuMeshInstanceBufferIndex = applicationPayload->GpuMeshInstanceBuffer.ReadDescriptor;
    applicationPayload->ShaderGlobalParameters.GpuMeshPrimitiveInstanceBufferIndex = applicationPayload->GpuMeshPrimitiveInstanceBuffer.ReadDescriptor;
    applicationPayload->ShaderGlobalParameters.Action = cameraState->Action;

    ElemUploadGraphicsBufferData(applicationPayload->ShaderGlobalParametersBuffer.Buffer, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderGlobalParameters, .Length = sizeof(ShaderShaderGlobalParameters) });
}

uint32_t test = 0;

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    test++;
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    
    if (updateParameters->SizeChanged)
    {
        CreateDepthBuffer(applicationPayload, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);
        CreateRenderTarget(applicationPayload, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);
    }

    ElemInputStream inputStream = ElemGetInputStream();

    SampleInputsApplicationUpdate(inputStream, &applicationPayload->InputsApplication, updateParameters->DeltaTimeInSeconds);
    SampleInputsCameraUpdate(inputStream, &applicationPayload->InputsCamera, updateParameters);

    if (updateParameters->SizeChanged || applicationPayload->InputsCamera.State.HasChanged || applicationPayload->InputsCamera.State.Action || !applicationPayload->UsePathTracingAccumulation)
    {
        applicationPayload->PathTracingSamplingCount = 0;
    }

    if (applicationPayload->InputsApplication.State.ExitApplication)
    {
        ElemExitApplication(0);
    }

    if (applicationPayload->InputsApplication.State.ShowCursor)
    {
        printf("Show cursor\n");
        ElemShowWindowCursor(applicationPayload->Window);
    }
    else if (applicationPayload->InputsApplication.State.HideCursor)
    {
        printf("Hide cursor\n");
        ElemHideWindowCursor(applicationPayload->Window); 
    } 

    SampleInputsCameraState* inputsCameraState = &applicationPayload->InputsCamera.State;
    UpdateShaderGlobalParameters(applicationPayload, inputsCameraState);
    applicationPayload->PathTraceLength = max(min(applicationPayload->PathTraceLength + inputsCameraState->Counter, 5), 1);

    if (inputsCameraState->Action)
    {
        applicationPayload->UsePathTracing = !applicationPayload->UsePathTracing;
    }

    if (inputsCameraState->Action2)
    {
        applicationPayload->UsePathTracingAccumulation = !applicationPayload->UsePathTracingAccumulation;
    }

    if (inputsCameraState->Action3)
    {
        applicationPayload->UseAnimation = !applicationPayload->UseAnimation;
    }

    ElemCommandList commandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL); 

    if (applicationPayload->UseAnimation)
    {
        SampleSceneNodeHeader* node = &applicationPayload->TestSceneData.Nodes[applicationPayload->TestSceneData.NodeCount - 1];
        node->Translation.Y += updateParameters->DeltaTimeInSeconds * applicationPayload->AnimationDirection;
        applicationPayload->PathTracingSamplingCount = 0;

        if (node->Translation.Y > 1.0f || node->Translation.Y < 0.0f)
        {
            applicationPayload->AnimationDirection = -applicationPayload->AnimationDirection;
        }
    
        BuildRaytracingTlas(commandList, applicationPayload);
    }

    ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters) {
        .RenderTargets = 
        {
            .Items = (ElemRenderPassRenderTarget[]) { 
                {
                    .RenderTarget = applicationPayload->RenderTargetTexture,
                    .LoadAction = applicationPayload->PathTracingSamplingCount ? ElemRenderPassLoadAction_Load : ElemRenderPassLoadAction_Clear,
                }},
            .Length = 1
        },
        .DepthStencil =
        {
            .DepthStencil = !applicationPayload->UsePathTracing ? applicationPayload->DepthBuffer : ELEM_HANDLE_NULL
        }
    });

    if (applicationPayload->UsePathTracing)
    {
        applicationPayload->PathTracingSamplingCount++;
        ElemBindPipelineState(commandList, applicationPayload->RaytracingGraphicsPipeline); 
    
        RaytracingShaderParameters parameters = 
        {
            .AccelerationStructureIndex = applicationPayload->TlasReadDescriptor,
            .ShaderGlobalParametersBufferIndex = applicationPayload->ShaderGlobalParametersBuffer.ReadDescriptor,
            // TODO: To Replace
            //.FrameIndex = updateParameters->FrameIndex
            .FrameIndex = test,
            .PathTraceLength = applicationPayload->PathTraceLength
        };

        ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&parameters, .Length = sizeof(RaytracingShaderParameters) });
        ElemDispatchMesh(commandList, 1, 1, 1);
    }
    else
    {
        applicationPayload->PathTracingSamplingCount = 1;
        ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline); 
        ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });

        for (uint32_t i = 0; i < applicationPayload->GpuMeshPrimitiveInstanceCount; i++)
        {
            applicationPayload->ShaderParameters.MeshPrimitiveInstanceId = i;

            ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });
            ElemDispatchMesh(commandList, applicationPayload->GpuMeshPrimitiveMeshletCountList[i], 1, 1);
        }
    }

    ElemEndRenderPass(commandList);

    ElemGraphicsResourceBarrier(commandList, applicationPayload->RenderTargetTextureReadDescriptor, NULL);

    ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters) {
        .RenderTargets = 
        {
            .Items = (ElemRenderPassRenderTarget[]) { 
            {
                .RenderTarget = updateParameters->BackBufferRenderTarget,
                .ClearColor = { 0.0f, 0.01f, 0.02f, 1.0f },
                .LoadAction = ElemRenderPassLoadAction_Clear
            }},
            .Length = 1
        }
    });
        
    ToneMapShaderParameters parameters = 
    {
        .SourceTexture = applicationPayload->RenderTargetTextureReadDescriptor,
        .SampleCount = applicationPayload->PathTracingSamplingCount
    };

    ElemBindPipelineState(commandList, applicationPayload->ToneMapGraphicsPipeline); 
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&parameters, .Length = sizeof(ToneMapShaderParameters) });

    ElemDispatchMesh(commandList, 1, 1, 1);

    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);
    applicationPayload->LastExecutionFence = ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, NULL);

    ElemPresentSwapChain(applicationPayload->SwapChain);
    SampleFrameMeasurement frameMeasurement = SampleEndFrameMeasurement();

    if (frameMeasurement.HasNewData)
    {
        SampleSetWindowTitle(applicationPayload->Window, "Hello Raytracing", applicationPayload->GraphicsDevice, frameMeasurement.FrameTimeInSeconds, frameMeasurement.Fps);
    }
    
    SampleStartFrameMeasurement();
}

int main(int argc, const char* argv[]) 
{
    ApplicationPayload payload =
    {
        .AppSettings = SampleParseAppSettings(argc, argv)
    };

    ElemConfigureLogHandler(ElemConsoleLogHandler);

    ElemRunApplication(&(ElemRunApplicationParameters)
    {
        .ApplicationName = "Hello Raytracing",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });
}
