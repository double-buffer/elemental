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
    uint32_t MeshletCount;
    uint32_t VertexBufferOffset;
    uint32_t MeshletOffset;
    uint32_t MeshletVertexIndexOffset;
    uint32_t MeshletTriangleIndexOffset;
    uint32_t ShowMeshlets;
} ShaderParameters;

typedef struct
{
    SampleMatrix4x4 ViewProjMatrix;
} ShaderFrameData;

// TODO: Group common variables into separate structs
typedef struct
{
    SampleAppSettings AppSettings;
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

// TODO: Copy paste the loading scene code from the common header because it is the first sample to explain how to load a mesh

void UpdateFrameData(ApplicationPayload* applicationPayload, SampleMatrix4x4 viewProjMatrix)
{
    applicationPayload->FrameData.ViewProjMatrix = viewProjMatrix;
    
    ElemDataSpan vertexBufferPointer = ElemGetGraphicsResourceDataSpan(applicationPayload->FrameDataBuffer.Buffer);
    memcpy(vertexBufferPointer.Items, &applicationPayload->FrameData, sizeof(ShaderFrameData));
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
    applicationPayload->GpuMemory = SampleCreateGpuMemory(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64));

    applicationPayload->FrameDataBuffer = SampleCreateGpuBufferAndUploadData(&applicationPayload->GpuMemory, &applicationPayload->FrameData, sizeof(ShaderFrameData), "FrameData");
    applicationPayload->ShaderParameters.FrameDataBuffer = applicationPayload->FrameDataBuffer.ReadDescriptor;

    CreateDepthBuffer(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);
    SampleLoadScene("sponza.scene", &applicationPayload->GpuMemory, &applicationPayload->TestSceneData);

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
 
    ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    ElemFreeGraphicsHeap(applicationPayload->DepthBufferHeap);

    SampleFreeGpuMemory(&applicationPayload->GpuMemory);
    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);

    SavedState savedState = { .CameraState = applicationPayload->InputsCamera.State }; 
    SampleWriteDataToApplicationFile("SavedState.bin", (ElemDataSpan) { .Items = (uint8_t*)&savedState, .Length = sizeof(SavedState) }, false);
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
    
    // TODO: Use another type of camera
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

    applicationPayload->ShaderParameters.ShowMeshlets = inputsCameraState->Action;

    UpdateFrameData(applicationPayload, inputsCameraState->ViewProjMatrix);

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

    // TODO: Replace that all here is really bad
    for (uint32_t i = 0; i < applicationPayload->TestSceneData.MeshCount; i++)
    {
        SampleMeshData* meshData = &applicationPayload->TestSceneData.Meshes[i];
        applicationPayload->ShaderParameters.MeshBuffer = meshData->MeshBuffer.ReadDescriptor;

        for (uint32_t j = 0; j < meshData->MeshPartCount; j++)
        {
            SampleMeshPartHeader* meshPart = &meshData->MeshParts[j];
            applicationPayload->ShaderParameters.MeshletCount = meshPart->MeshletCount;
            applicationPayload->ShaderParameters.VertexBufferOffset = meshPart->VertexBufferOffset;
            applicationPayload->ShaderParameters.MeshletOffset = meshPart->MeshletOffset;
            applicationPayload->ShaderParameters.MeshletVertexIndexOffset = meshPart->MeshletVertexIndexOffset;
            applicationPayload->ShaderParameters.MeshletTriangleIndexOffset = meshPart->MeshletTriangleIndexOffset;

            ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });
            ElemDispatchMesh(commandList, meshPart->MeshletCount, 1, 1);
        }
    }

    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);
    applicationPayload->LastExecutionFence = ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, NULL);

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
        .AppSettings = SampleParseAppSettings(argc, argv)
    };

    // TODO: Load and save state

    ElemConfigureLogHandler(ElemConsoleLogHandler);

    ElemRunApplication(&(ElemRunApplicationParameters)
    {
        .ApplicationName = "Renderer",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });

}
