#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"
#include "SampleInputsApplication.h"
#include "SampleInputsCamera.h"
#include "SampleSceneLoader.h"
#include "SampleGpuMemory.h"

// TODO: Share data between shader and C code

typedef struct
{
    uint32_t ShaderGlobalParametersBuffer;
    uint32_t MeshPrimitiveInstanceId;
} ShaderParameters;

typedef struct
{
    SampleMatrix4x4 ViewProjMatrix;
    SampleMatrix4x4 InverseViewMatrix;
    SampleMatrix4x4 InverseProjectionMatrix;
    uint32_t MaterialBufferIndex;
    uint32_t MeshInstanceBufferIndex;
    uint32_t MeshPrimitiveInstanceBufferIndex;
    uint32_t TextureSampler;
    uint32_t Action;
} ShaderShaderGlobalParameters;

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
} GpuSceneData;

// TODO: Group common variables into separate structs
typedef struct
{
    SampleAppSettings AppSettings;
    const char* ScenePath;

    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;

    SampleGpuMemory GpuMemory;
    SampleGpuMemory GpuMemoryUpload;

    ElemFence LastExecutionFence;
    ElemSwapChain SwapChain;
    ElemGraphicsHeap DepthBufferHeap;
    ElemGraphicsResource DepthBuffer;
    ElemPipelineState GraphicsPipeline;
    ShaderParameters ShaderParameters;
    SampleInputsApplication InputsApplication;
    SampleInputsCamera InputsCamera;
    SampleSceneData TestSceneData; // TODO: Do we keep that structure here?

    ShaderShaderGlobalParameters ShaderGlobalParameters;
    SampleGpuBuffer ShaderGlobalParametersBuffer;

    GpuSceneData GpuSceneData;
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

void UpdateShaderGlobalParameters(ApplicationPayload* applicationPayload, const SampleInputsCameraState* cameraState)
{
    applicationPayload->ShaderGlobalParameters.ViewProjMatrix = cameraState->ViewProjMatrix;
    applicationPayload->ShaderGlobalParameters.InverseViewMatrix = cameraState->InverseViewMatrix;
    applicationPayload->ShaderGlobalParameters.InverseProjectionMatrix = cameraState->InverseProjectionMatrix;
    applicationPayload->ShaderGlobalParameters.MaterialBufferIndex = applicationPayload->GpuSceneData.MaterialBuffer.ReadDescriptor;
    applicationPayload->ShaderGlobalParameters.MeshInstanceBufferIndex = applicationPayload->GpuSceneData.MeshInstanceBuffer.ReadDescriptor;
    applicationPayload->ShaderGlobalParameters.MeshPrimitiveInstanceBufferIndex = applicationPayload->GpuSceneData.MeshPrimitiveInstanceBuffer.ReadDescriptor;

    if (cameraState->Action)
    {
        applicationPayload->ShaderGlobalParameters.Action = !applicationPayload->ShaderGlobalParameters.Action;
    }

    ElemUploadGraphicsBufferData(applicationPayload->ShaderGlobalParametersBuffer.Buffer, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderGlobalParameters, .Length = sizeof(ShaderShaderGlobalParameters) });
}

void LoadGpuSceneData(ElemCommandList commandList, const SampleSceneData* sceneData, GpuSceneData* gpuSceneData, SampleGpuMemory* gpuMemory)
{
    // TODO: Refactor/Optimize this function

    uint32_t gpuMeshInstanceCount = 0u;
    uint32_t gpuMeshPrimitiveInstanceCount = 0u;

    gpuSceneData->Textures = (SampleGpuTexture*)malloc(sizeof(SampleGpuTexture) * sceneData->TextureCount);
    gpuSceneData->TextureCount = sceneData->TextureCount;

    for (uint32_t i = 0; i < sceneData->TextureCount; i++)
    {
        SampleTextureData* textureData = &sceneData->Textures[i];
        SampleGpuTexture* texture = &gpuSceneData->Textures[i];

        ElemGraphicsFormat format = !textureData->IsNormalTexture ? ElemGraphicsFormat_BC7_SRGB : ElemGraphicsFormat_BC7;
        *texture = SampleCreateGpuTexture(gpuMemory, textureData->TextureHeader.Width, textureData->TextureHeader.Height, textureData->TextureHeader.MipCount, format, textureData->Path);

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
                .Resource = texture->Texture,
                .TextureMipLevel = i,
                .SourceType = ElemCopyDataSourceType_Memory,
                .SourceMemoryData = { .Items = (uint8_t*)mipData, .Length = mipEntry.SizeInBytes } 
            };

            ElemCopyDataToGraphicsResource(commandList, &copyParameters);
            free(mipData);
        }
    }

    ShaderMaterial* shaderMaterials = (ShaderMaterial*)malloc(sizeof(ShaderMaterial) * sceneData->MaterialCount);

    for (uint32_t i = 0; i < sceneData->MaterialCount; i++)
    {
        SampleSceneMaterialHeader* materialHeader = &sceneData->Materials[i];
        ShaderMaterial* shaderMaterial = &shaderMaterials[i];

        shaderMaterial->AlbedoFactor = materialHeader->AlbedoFactor;
        shaderMaterial->EmissiveFactor = materialHeader->EmissiveFactor;
        shaderMaterial->AlbedoTextureId = -1;
        shaderMaterial->NormalTextureId = -1;

        if (materialHeader->AlbedoTextureId >= 0)
        {
            shaderMaterial->AlbedoTextureId = gpuSceneData->Textures[materialHeader->AlbedoTextureId].ReadDescriptor;
        }

        if (materialHeader->NormalTextureId >= 0)
        {
            shaderMaterial->NormalTextureId = gpuSceneData->Textures[materialHeader->NormalTextureId].ReadDescriptor;
        }
    }

    gpuSceneData->MaterialBuffer = SampleCreateGpuBuffer(gpuMemory, sceneData->MaterialCount * sizeof(ShaderMaterial), "MaterialBuffer");

    ElemCopyDataToGraphicsResourceParameters copyParameters =
    {
        .Resource = gpuSceneData->MaterialBuffer.Buffer,
        .SourceType = ElemCopyDataSourceType_Memory,
        .SourceMemoryData = { .Items = (uint8_t*)shaderMaterials, .Length = sceneData->MaterialCount * sizeof(ShaderMaterial) } 
    };

    ElemCopyDataToGraphicsResource(commandList, &copyParameters);

    free(shaderMaterials);

    gpuSceneData->MeshBuffers = (SampleGpuBuffer*)malloc(sizeof(SampleGpuBuffer) * sceneData->MeshCount);
    gpuSceneData->MeshCount = sceneData->MeshCount;

    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleMeshData* meshData = &sceneData->Meshes[i];

        SampleGpuBuffer* meshBuffer = &gpuSceneData->MeshBuffers[i];
        *meshBuffer = SampleCreateGpuBuffer(gpuMemory, meshData->MeshHeader.MeshBufferSizeInBytes, meshData->MeshHeader.Name);

        // TODO: this will be simplified when file io will be implemented
        assert(meshData->Path);
        uint8_t* meshBufferData = (uint8_t*)malloc(sizeof(uint8_t) * meshData->MeshHeader.MeshBufferSizeInBytes);

        FILE* file = SampleOpenFile(meshData->Path, true);
        assert(file);

        fseek(file, meshData->MeshHeader.MeshBufferOffset, SEEK_SET);
        fread(meshBufferData, sizeof(uint8_t), meshData->MeshHeader.MeshBufferSizeInBytes, file);
        fclose(file);

        ElemCopyDataToGraphicsResourceParameters copyParameters =
        {
            .Resource = meshBuffer->Buffer,
            .SourceType = ElemCopyDataSourceType_Memory,
            .SourceMemoryData = { .Items = (uint8_t*)meshBufferData, .Length = meshData->MeshHeader.MeshBufferSizeInBytes } 
        };

        ElemCopyDataToGraphicsResource(commandList, &copyParameters);
        free(meshBufferData);
    }

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

    gpuSceneData->MeshInstanceBuffer = SampleCreateGpuBuffer(gpuMemory, gpuMeshInstanceCount * sizeof(GpuMeshInstance), "GpuMeshInstanceBuffer");

    copyParameters = (ElemCopyDataToGraphicsResourceParameters)
    {
        .Resource = gpuSceneData->MeshInstanceBuffer.Buffer,
        .SourceType = ElemCopyDataSourceType_Memory,
        .SourceMemoryData = { .Items = (uint8_t*)gpuMeshInstancesData, .Length = gpuMeshInstanceCount * sizeof(GpuMeshInstance) } 
    };

    ElemCopyDataToGraphicsResource(commandList, &copyParameters);

    gpuSceneData->MeshPrimitiveInstanceBuffer = SampleCreateGpuBuffer(gpuMemory, gpuMeshPrimitiveInstanceCount * sizeof(GpuMeshPrimitiveInstance), "GpuMeshPrimitiveInstanceBuffer");

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

void FreeGpuSceneData(GpuSceneData* gpuSceneData)
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

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->Window = ElemCreateWindow(&(ElemWindowOptions) { .WindowState = applicationPayload->AppSettings.PreferFullScreen ? ElemWindowState_FullScreen : ElemWindowState_Normal });

    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = !applicationPayload->AppSettings.DisableDiagnostics, .EnableGpuValidation = false, .EnableDebugBarrierInfo = false, .PreferVulkan = applicationPayload->AppSettings.PreferVulkan });
    
    applicationPayload->GraphicsDevice = ElemCreateGraphicsDevice(NULL);

    applicationPayload->CommandQueue= ElemCreateCommandQueue(applicationPayload->GraphicsDevice, ElemCommandQueueType_Graphics, NULL);
    applicationPayload->SwapChain= ElemCreateSwapChain(applicationPayload->CommandQueue, applicationPayload->Window, UpdateSwapChain, &(ElemSwapChainOptions) { .FrameLatency = 1, .UpdatePayload = payload });
    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(applicationPayload->SwapChain);

    // TODO: For now we create a separate heap to avoid memory management
    applicationPayload->DepthBufferHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_Gpu });

    // TODO: For now we need to put the heap as GpuUpload but it should be Gpu when we use IOQueues
    // TODO: Having GPU Upload is still annoying 😞
    // TODO: For the moment we implement all textures of the scene in GPU memory. That is why we need 2GB for bistro scene
    // We will implement virtual texturing/texture streaming in the future
    applicationPayload->GpuMemory = SampleCreateGpuMemory(applicationPayload->GraphicsDevice, ElemGraphicsHeapType_Gpu, SampleMegaBytesToBytes(2048));
    applicationPayload->GpuMemoryUpload = SampleCreateGpuMemory(applicationPayload->GraphicsDevice, ElemGraphicsHeapType_GpuUpload, SampleMegaBytesToBytes(64));

    applicationPayload->ShaderGlobalParametersBuffer = SampleCreateGpuBufferAndUploadData(&applicationPayload->GpuMemoryUpload, &applicationPayload->ShaderGlobalParameters, sizeof(ShaderShaderGlobalParameters), "ShaderGlobalParameters");
    applicationPayload->ShaderParameters.ShaderGlobalParametersBuffer = applicationPayload->ShaderGlobalParametersBuffer.ReadDescriptor;

    // TODO: Do we need the scene data after that?
    CreateDepthBuffer(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);
    SampleLoadScene(applicationPayload->ScenePath, &applicationPayload->TestSceneData, &applicationPayload->GpuMemoryUpload);

    ElemGraphicsSamplerInfo samplerInfo =
    {
        .MinFilter = ElemGraphicsSamplerFilter_Linear,
        .MagFilter = ElemGraphicsSamplerFilter_Linear,
        .MipFilter = ElemGraphicsSamplerFilter_Linear,
        .MaxAnisotropy = 16,
    };

    applicationPayload->ShaderGlobalParameters.TextureSampler = ElemCreateGraphicsSampler(applicationPayload->GraphicsDevice, &samplerInfo);

    ElemDataSpan shaderData = SampleReadFile(!applicationPayload->AppSettings.PreferVulkan ? "RenderMesh.shader": "RenderMesh_vulkan.shader", true);
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, shaderData);

    applicationPayload->GraphicsPipeline = ElemCompileGraphicsPipelineState(applicationPayload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "RenderMesh PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        //.CullMode = ElemGraphicsCullMode_None, // TODO: We need to deactivate cull only for transparent objects!
        .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {{ .Format = swapChainInfo.Format }}, .Length = 1 },
        .DepthStencil =
        {
            .Format = ElemGraphicsFormat_D32_FLOAT,
            .DepthCompareFunction = ElemGraphicsCompareFunction_Greater
        }
    });

    ElemFreeShaderLibrary(shaderLibrary);

    SampleInputsApplicationInit(&applicationPayload->InputsApplication);
    SampleInputsCameraInit(&applicationPayload->InputsCamera);
    
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

    FreeGpuSceneData(&applicationPayload->GpuSceneData);
    SampleFreeScene(&applicationPayload->TestSceneData);
    SampleFreeGpuBuffer(&applicationPayload->ShaderGlobalParametersBuffer);

    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
 
    ElemFreeGraphicsSampler(applicationPayload->ShaderGlobalParameters.TextureSampler, NULL);
    ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    ElemFreeGraphicsHeap(applicationPayload->DepthBufferHeap);

    SampleFreeGpuMemory(&applicationPayload->GpuMemoryUpload);
    SampleFreeGpuMemory(&applicationPayload->GpuMemory);
    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);

    SavedState savedState = { .CameraState = applicationPayload->InputsCamera.State }; 
    SampleWriteDataToApplicationFile("SavedState.bin", (ElemDataSpan) { .Items = (uint8_t*)&savedState, .Length = sizeof(SavedState) }, false);

    printf("Exit application...\n");
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    
    if (updateParameters->SizeChanged)
    {
        CreateDepthBuffer(applicationPayload, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);
    }

    ElemInputStream inputStream = ElemGetInputStream();

    SampleInputsApplicationUpdate(inputStream, &applicationPayload->InputsApplication, updateParameters->DeltaTimeInSeconds);
    SampleInputsCameraUpdate(inputStream, &applicationPayload->InputsCamera, updateParameters);

    // TODO: We shold move this into the application update function (we can pass the window if needed)
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

    // TODO: We need to have a kind of queue system. The problem here is that if we don't have any
    // data to load we will create empty lists

    ElemFence loadDataFence = {};

    if (!applicationPayload->GpuSceneData.IsLoaded)
    {
        // TODO: We should have a queue system instead and load only what is needed otherwise
        // each frames we will have empty lists
        // This system will allow to split by batches the loading
        ElemCommandList loadDataCommandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL);

        LoadGpuSceneData(loadDataCommandList, &applicationPayload->TestSceneData, &applicationPayload->GpuSceneData, &applicationPayload->GpuMemory);
        ElemCommitCommandList(loadDataCommandList);    // TODO: Measure scene loading time 

        loadDataFence = ElemExecuteCommandList(applicationPayload->CommandQueue, loadDataCommandList, NULL);
    }

    ElemCommandList commandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL); 

    ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters) {
        .RenderTargets = 
        {
            .Items = (ElemRenderPassRenderTarget[]) { 
            {
                .RenderTarget = updateParameters->BackBufferRenderTarget,
                .ClearColor = { 0.0f, 0.01f, 0.02f, 1.0f },
            }},
            .Length = 1
        },
        .DepthStencil =
        {
            .DepthStencil = applicationPayload->DepthBuffer
        }
    });

    ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline); 

    // TODO: Construct a list of tasks on the cpu for now and do only one dispatch mesh with the total of tasks
    // Be carreful with the limit per dimension of 65000
    ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline); 
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });

    for (uint32_t i = 0; i < applicationPayload->GpuSceneData.MeshPrimitiveInstanceCount; i++)
    {
        applicationPayload->ShaderParameters.MeshPrimitiveInstanceId = i;

        ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });
        ElemDispatchMesh(commandList, applicationPayload->GpuSceneData.MeshPrimitiveMeshletCountList[i], 1, 1);
    }

    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);

    ElemExecuteCommandListOptions executeOptions = {};

    if (loadDataFence.CommandQueue != ELEM_HANDLE_NULL)
    {
        // BUG: Sometime we have a crash on vulkan
        executeOptions.FencesToWait = (ElemFenceSpan){ .Items = &loadDataFence, .Length = 1 };
    }

    applicationPayload->LastExecutionFence = ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, &executeOptions);

    ElemPresentSwapChain(applicationPayload->SwapChain);
    SampleFrameMeasurement frameMeasurement = SampleEndFrameMeasurement();

    if (frameMeasurement.HasNewData)
    {
        SampleSetWindowTitle(applicationPayload->Window, "Renderer", applicationPayload->GraphicsDevice, frameMeasurement.FrameTimeInSeconds, frameMeasurement.Fps);
    }
    
    SampleStartFrameMeasurement();
}


int main(int argc, const char* argv[]) 
{
    ApplicationPayload payload =
    {
        .AppSettings = SampleParseAppSettings(argc, argv),
        .ScenePath = "Sponza/sponza.scene"
    };

    int32_t scenePathIndex = argc - 1;

    if (strstr(argv[scenePathIndex], ".scene"))
    {
        payload.ScenePath = argv[scenePathIndex];
    }

    ElemConfigureLogHandler(SampleConsoleAndFileLogHandler);

    ElemRunApplication(&(ElemRunApplicationParameters)
    {
        .ApplicationName = "Renderer",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });

}
