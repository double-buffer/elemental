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
    uint32_t FrameDataBuffer;
    // TODO: Embed that into a buffer
    uint32_t MeshBuffer;
    uint32_t MaterialBuffer;
    uint32_t VertexBufferOffset;
    uint32_t MeshletOffset;
    uint32_t MeshletVertexIndexOffset;
    uint32_t MeshletTriangleIndexOffset;
    float Scale;
    ElemVector3 Translation;
    uint32_t Reserved;
    ElemVector4 Rotation;
    uint32_t MaterialId;
    uint32_t TextureSampler;
} ShaderParameters;

typedef struct
{
    SampleMatrix4x4 ViewProjMatrix;
    uint32_t ShowMeshlets;
} ShaderFrameData;

// TODO: Group common variables into separate structs
typedef struct
{
    SampleAppSettings AppSettings;
    const char* ScenePath;

    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;

    SampleGpuMemory GpuMemory;

    ElemFence LastExecutionFence;
    ElemSwapChain SwapChain;
    ElemGraphicsHeap DepthBufferHeap;
    ElemGraphicsResource DepthBuffer;
    ElemPipelineState GraphicsPipeline;
    ShaderParameters ShaderParameters;
    SampleInputsApplication InputsApplication;
    SampleInputsCamera InputsCamera;
    SampleSceneData TestSceneData; // TODO: Do we keep that structure here?

    ShaderFrameData FrameData;
    SampleGpuBuffer FrameDataBuffer;
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

void UpdateFrameData(ApplicationPayload* applicationPayload, SampleMatrix4x4 viewProjMatrix, bool showMeshlets)
{
    applicationPayload->FrameData.ViewProjMatrix = viewProjMatrix;
    applicationPayload->FrameData.ShowMeshlets = showMeshlets;

    ElemUploadGraphicsBufferData(applicationPayload->FrameDataBuffer.Buffer, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->FrameData, .Length = sizeof(ShaderFrameData) });
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
    applicationPayload->GpuMemory = SampleCreateGpuMemory(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(2048));

    applicationPayload->FrameDataBuffer = SampleCreateGpuBufferAndUploadData(&applicationPayload->GpuMemory, &applicationPayload->FrameData, sizeof(ShaderFrameData), "FrameData");
    applicationPayload->ShaderParameters.FrameDataBuffer = applicationPayload->FrameDataBuffer.ReadDescriptor;

    CreateDepthBuffer(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);
    SampleLoadScene(applicationPayload->ScenePath, &applicationPayload->TestSceneData, &applicationPayload->GpuMemory);

    ElemGraphicsSamplerInfo samplerInfo =
    {
        .MinFilter = ElemGraphicsSamplerFilter_Linear,
        .MagFilter = ElemGraphicsSamplerFilter_Linear,
        .MipFilter = ElemGraphicsSamplerFilter_Linear,
        .MaxAnisotropy = 16,
    };

    applicationPayload->ShaderParameters.TextureSampler = ElemCreateGraphicsSampler(applicationPayload->GraphicsDevice, &samplerInfo);

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

    SampleFreeScene(&applicationPayload->TestSceneData);
    SampleFreeGpuBuffer(&applicationPayload->FrameDataBuffer);

    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
 
    ElemFreeGraphicsSampler(applicationPayload->ShaderParameters.TextureSampler, NULL);
    ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    ElemFreeGraphicsHeap(applicationPayload->DepthBufferHeap);

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

    UpdateFrameData(applicationPayload, inputsCameraState->ViewProjMatrix, inputsCameraState->Action);

    // TODO: We need to have a kind of queue system. The problem here is that if we don't have any
    // data to load we will create empty lists

    ElemFence loadDataFence = {};

    // TODO: Fow now, we load all the material textures in one shot
    // We do it in the main loop now because later it will be a streaming system.
    // Doing it in the main loop force us to decouple the loading
    uint32_t loadResourceCounter = 0;

    for (uint32_t i = 0; i < applicationPayload->TestSceneData.MaterialCount; i++)
    {
        SampleMaterialData* material = &applicationPayload->TestSceneData.Materials[i];

        if (material->AlbedoTexture && !material->AlbedoTexture->IsLoaded)
        {
            loadResourceCounter++;
        }

        if (material->NormalTexture && !material->NormalTexture->IsLoaded)
        {
            loadResourceCounter++;
        }
    }

    for (uint32_t i = 0; i < applicationPayload->TestSceneData.MeshCount; i++)
    {
        SampleMeshData* meshData = &applicationPayload->TestSceneData.Meshes[i];

        if (meshData->MeshBuffer.Buffer == ELEM_HANDLE_NULL)
        { 
            loadResourceCounter++;
        }
    }

    if (loadResourceCounter > 0)
    {
        // TODO: We should have a queue system instead and load only what is needed otherwise
        // each frames we will have empty lists
        // This system will allow to split by batches the loading
        ElemCommandList loadDataCommandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL);

        for (uint32_t i = 0; i < applicationPayload->TestSceneData.MaterialCount; i++)
        {
            SampleMaterialData* material = &applicationPayload->TestSceneData.Materials[i];

            if (material->AlbedoTexture && !material->AlbedoTexture->IsLoaded)
            {
                SampleLoadTextureData(loadDataCommandList, material->AlbedoTexture, &applicationPayload->GpuMemory);
                loadResourceCounter++;
            }

            if (material->NormalTexture && !material->NormalTexture->IsLoaded)
            {
                SampleLoadTextureData(loadDataCommandList, material->NormalTexture, &applicationPayload->GpuMemory);
                loadResourceCounter++;
            }
        }

        for (uint32_t i = 0; i < applicationPayload->TestSceneData.NodeCount; i++)
        {
            SampleSceneNodeHeader* sceneNode = &applicationPayload->TestSceneData.Nodes[i];

            if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
            {
                SampleMeshData* meshData = &applicationPayload->TestSceneData.Meshes[sceneNode->ReferenceIndex];

                if (meshData->MeshBuffer.Buffer == ELEM_HANDLE_NULL)
                { 
                    // TODO: This is an attempt to dissociate the loading of the mesh metadata and the buffer data
                    // Later, this will be done via streaming in parrallel. The gpu shader that will handle the mesh will need to check
                    // if the data was loaded.
                    // For now, we block to load the mesh if not already done which is equavalent at loading the full scene during the init.
                    SampleLoadMeshData(loadDataCommandList, meshData, &applicationPayload->GpuMemory);
                    loadResourceCounter++;
                }
            }
        }

        ElemCommitCommandList(loadDataCommandList);    // TODO: Measure scene loading time 

        printf("ResourceLoadCount: %d\n", loadResourceCounter);
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

    applicationPayload->ShaderParameters.MaterialBuffer = applicationPayload->TestSceneData.MaterialBuffer.ReadDescriptor;

    // TODO: Construct a list of tasks on the cpu for now and do only one dispatch mesh with the total of tasks
    // Be carreful with the limit per dimension of 65000
    for (uint32_t i = 0; i < applicationPayload->TestSceneData.NodeCount; i++)
    {
        SampleSceneNodeHeader* sceneNode = &applicationPayload->TestSceneData.Nodes[i];

        if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
        {
            SampleMeshData* meshData = &applicationPayload->TestSceneData.Meshes[sceneNode->ReferenceIndex];
            
            // TODO: We need to check if the data is loaded into the gpu

            applicationPayload->ShaderParameters.MeshBuffer = meshData->MeshBuffer.ReadDescriptor;

            for (uint32_t j = 0; j < meshData->MeshHeader.MeshPrimitiveCount; j++)
            {
                SampleMeshPrimitiveHeader* meshPrimitive = &meshData->MeshPrimitives[j];
                applicationPayload->ShaderParameters.VertexBufferOffset = meshPrimitive->VertexBufferOffset;
                applicationPayload->ShaderParameters.MeshletOffset = meshPrimitive->MeshletOffset;
                applicationPayload->ShaderParameters.MeshletVertexIndexOffset = meshPrimitive->MeshletVertexIndexOffset;
                applicationPayload->ShaderParameters.MeshletTriangleIndexOffset = meshPrimitive->MeshletTriangleIndexOffset;
                applicationPayload->ShaderParameters.Scale = sceneNode->Scale;
                applicationPayload->ShaderParameters.Translation = sceneNode->Translation;
                applicationPayload->ShaderParameters.MaterialId = meshPrimitive->MaterialId;
                applicationPayload->ShaderParameters.Rotation = sceneNode->Rotation;

                ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });
                ElemDispatchMesh(commandList, meshPrimitive->MeshletCount, 1, 1);
            }
        }
    }

    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);

    ElemExecuteCommandListOptions executeOptions = {};

    if (loadResourceCounter > 0)
    {
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

    ElemConfigureLogHandler(ElemConsoleLogHandler);

    ElemRunApplication(&(ElemRunApplicationParameters)
    {
        .ApplicationName = "Renderer",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });

}
