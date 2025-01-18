#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"
#include "SampleInputsApplication.h"
#include "SampleInputsCamera.h"
#include "SampleSceneLoader.h"

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
    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    uint32_t CurrentHeapOffset;
    ElemFence LastExecutionFence;
    ElemSwapChain SwapChain;
    ElemGraphicsHeap DepthBufferHeap;
    ElemGraphicsResource DepthBuffer;
    ElemPipelineState GraphicsPipeline;
    ElemPipelineState RaytracingGraphicsPipeline;
    ShaderParameters ShaderParameters;
    SampleInputsApplication InputsApplication;
    SampleInputsCamera InputsCamera;
    SampleSceneData TestSceneData;
    SampleGpuMemory GpuMemory;

    ShaderFrameData FrameData;
    SampleGpuBuffer FrameDataBuffer;
    
    SampleGpuTexture RenderTargetTexture;
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
    if (applicationPayload->RenderTargetTexture.Texture != ELEM_HANDLE_NULL)
    {
        SampleFreeGpuTexture(&applicationPayload->RenderTargetTexture);
    }

    printf("Creating render texture...\n");

    applicationPayload->RenderTargetTexture = SampleCreateGpuRenderTarget(&applicationPayload->GpuMemory, width, height, ElemGraphicsFormat_R32G32B32A32_FLOAT, "RenderTarget");
}

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->Window = ElemCreateWindow(&(ElemWindowOptions) { .WindowState = applicationPayload->AppSettings.PreferFullScreen ? ElemWindowState_FullScreen : ElemWindowState_Normal });

    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = !applicationPayload->AppSettings.DisableDiagnostics, .EnableGpuValidation = false, .EnableDebugBarrierInfo = false, .PreferVulkan = applicationPayload->AppSettings.PreferVulkan });
    
    // TODO: Debug why the AMD integrated GPU is not create at all
    ElemGraphicsDeviceInfoSpan devices = ElemGetAvailableGraphicsDevices();
    
    for (uint32_t i = 0; i < devices.Length; i++)
    {
        printf("Device: %s\n", devices.Items[i].DeviceName);
    }

    applicationPayload->GraphicsDevice = ElemCreateGraphicsDevice(NULL);

    applicationPayload->CommandQueue= ElemCreateCommandQueue(applicationPayload->GraphicsDevice, ElemCommandQueueType_Graphics, NULL);
    applicationPayload->SwapChain= ElemCreateSwapChain(applicationPayload->CommandQueue, applicationPayload->Window, UpdateSwapChain, &(ElemSwapChainOptions) { .FrameLatency = 1, .UpdatePayload = payload });
    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(applicationPayload->SwapChain);

    // TODO: For now we create a separate heap to avoid memory management
    applicationPayload->DepthBufferHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_Gpu });
    applicationPayload->GpuMemory = SampleCreateGpuMemory(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(256));

    CreateDepthBuffer(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);
    CreateRenderTarget(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);
    SampleLoadScene("CornellBox.scene", &applicationPayload->TestSceneData, &applicationPayload->GpuMemory);
    
    applicationPayload->FrameDataBuffer = SampleCreateGpuBufferAndUploadData(&applicationPayload->GpuMemory, &applicationPayload->FrameData, sizeof(ShaderFrameData), "FrameData");
    applicationPayload->ShaderParameters.FrameDataBuffer = applicationPayload->FrameDataBuffer.ReadDescriptor;

    ElemCommandList loadDataCommandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL);
    
    for (uint32_t i = 0; i < applicationPayload->TestSceneData.NodeCount; i++)
    {
        SampleSceneNodeHeader* sceneNode = &applicationPayload->TestSceneData.Nodes[i];

        if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
        {
            SampleMeshData* meshData = &applicationPayload->TestSceneData.Meshes[sceneNode->ReferenceIndex];

            if (meshData->MeshBuffer.Buffer == ELEM_HANDLE_NULL)
            { 
                SampleLoadMeshData(loadDataCommandList, meshData, &applicationPayload->GpuMemory);
            }
        }
    }

    ElemCommitCommandList(loadDataCommandList);
    ElemExecuteCommandList(applicationPayload->CommandQueue, loadDataCommandList, NULL);

    ElemDataSpan shaderData = SampleReadFile(!applicationPayload->AppSettings.PreferVulkan ? "RenderMesh.shader": "RenderMesh_vulkan.shader", true);
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, shaderData);

    applicationPayload->GraphicsPipeline = ElemCompileGraphicsPipelineState(applicationPayload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "RenderMesh PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {{ .Format = swapChainInfo.Format }}, .Length = 1 },
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
        .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {{ .Format = swapChainInfo.Format }}, .Length = 1 },
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

    SampleFreeGpuTexture(&applicationPayload->RenderTargetTexture);
    SampleFreeScene(&applicationPayload->TestSceneData);
    SampleFreeGpuBuffer(&applicationPayload->FrameDataBuffer);

    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreePipelineState(applicationPayload->RaytracingGraphicsPipeline);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
 
    ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    ElemFreeGraphicsHeap(applicationPayload->DepthBufferHeap);

    SampleFreeGpuMemory(&applicationPayload->GpuMemory);

    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);
    
    SavedState savedState = { .CameraState = applicationPayload->InputsCamera.State }; 
    SampleWriteDataToApplicationFile("SavedState.bin", (ElemDataSpan) { .Items = (uint8_t*)&savedState, .Length = sizeof(SavedState) }, false);

    printf("Exit application...\n");
}

void UpdateFrameData(ApplicationPayload* applicationPayload, SampleMatrix4x4 viewProjMatrix, bool showMeshlets)
{
    applicationPayload->FrameData.ViewProjMatrix = viewProjMatrix;
    applicationPayload->FrameData.ShowMeshlets = showMeshlets;

    ElemUploadGraphicsBufferData(applicationPayload->FrameDataBuffer.Buffer, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->FrameData, .Length = sizeof(ShaderFrameData) });
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    
    if (updateParameters->SizeChanged)
    {
        CreateDepthBuffer(applicationPayload, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);
        CreateRenderTarget(applicationPayload, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);
    }

    ElemInputStream inputStream = ElemGetInputStream();

    SampleInputsApplicationUpdate(inputStream, &applicationPayload->InputsApplication, updateParameters->DeltaTimeInSeconds);
    SampleInputsCameraUpdate(inputStream, &applicationPayload->InputsCamera, updateParameters);

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

    if (!inputsCameraState->Action)
    {
        ElemBindPipelineState(commandList, applicationPayload->RaytracingGraphicsPipeline); 
        ElemDispatchMesh(commandList, 1, 1, 1);
    }
    else
    {
        ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline); 
        ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });

        for (uint32_t i = 0; i < applicationPayload->TestSceneData.NodeCount; i++)
        {
            SampleSceneNodeHeader* sceneNode = &applicationPayload->TestSceneData.Nodes[i];

            if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
            {
                SampleMeshData* meshData = &applicationPayload->TestSceneData.Meshes[sceneNode->ReferenceIndex];

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
    }

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
