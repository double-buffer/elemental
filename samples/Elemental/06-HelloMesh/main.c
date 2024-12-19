#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"
#include "SampleInputsApplication.h"
#include "SampleInputsModelViewer.h"
#include "SampleMesh.h"

typedef struct
{
    uint32_t VertexBuffer;
    uint32_t MeshletBuffer;
    uint32_t MeshletVertexIndexBuffer;
    uint32_t MeshletTriangleIndexBuffer;
    SampleVector4 RotationQuaternion;
    float Zoom;
    float AspectRatio;
    uint32_t ShowMeshlets;
    uint32_t MeshletCount;
} ShaderParameters;

typedef struct
{
    uint32_t MeshletCount;
    ElemGraphicsResource VertexBuffer;
    ElemGraphicsResourceDescriptor VertexBufferReadDescriptor;
    ElemGraphicsResource MeshletBuffer;
    ElemGraphicsResourceDescriptor MeshletBufferReadDescriptor;
    ElemGraphicsResource MeshletVertexIndexBuffer;
    ElemGraphicsResourceDescriptor MeshletVertexIndexBufferReadDescriptor;
    ElemGraphicsResource MeshletTriangleIndexBuffer;
    ElemGraphicsResourceDescriptor MeshletTriangleIndexBufferReadDescriptor;
} MeshData;

// TODO: Group common variables into separate structs
typedef struct
{
    SampleAppSettings AppSettings;
    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    ElemGraphicsHeap GraphicsHeap;
    uint32_t CurrentHeapOffset;
    ElemFence LastExecutionFence;
    ElemSwapChain SwapChain;
    ElemGraphicsHeap DepthBufferHeap;
    ElemGraphicsResource DepthBuffer;
    ElemPipelineState GraphicsPipeline;
    ShaderParameters ShaderParameters;
    SampleInputsApplication InputsApplication;
    SampleInputsModelViewer InputsModelViewer;
    MeshData TestMeshData;
} ApplicationPayload;
    
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

// TODO: To remove when IOQueues
void CreateAndUploadDataTemp(ElemGraphicsResource* buffer, ElemGraphicsResourceDescriptor* readDescriptor, ApplicationPayload* applicationPayload, void* dataPointer, uint32_t sizeInBytes)
{
    // TODO: Alignment should be used with the offset before adding the size of the resource!
    ElemGraphicsResourceInfo bufferDescription = ElemCreateGraphicsBufferResourceInfo(applicationPayload->GraphicsDevice, sizeInBytes, ElemGraphicsResourceUsage_Read, NULL);

    applicationPayload->CurrentHeapOffset = SampleAlignValue(applicationPayload->CurrentHeapOffset, bufferDescription.Alignment);
    *buffer = ElemCreateGraphicsResource(applicationPayload->GraphicsHeap, applicationPayload->CurrentHeapOffset, &bufferDescription);
    applicationPayload->CurrentHeapOffset += bufferDescription.SizeInBytes;

    *readDescriptor = ElemCreateGraphicsResourceDescriptor(*buffer, ElemGraphicsResourceDescriptorUsage_Read, NULL);

    ElemDataSpan vertexBufferPointer = ElemGetGraphicsResourceDataSpan(*buffer);
    memcpy(vertexBufferPointer.Items, dataPointer, sizeInBytes);
}

void LoadMesh(MeshData* meshData, const char* path, ApplicationPayload* applicationPayload)
{
    // TODO: When IOQueues are implemented, we only need to read the header, not the whole file!
    // Add a parameter to specify the length we want to read and the offset
    ElemDataSpan meshFileData = SampleReadFile(path);

    uint8_t* fileDataPointer = meshFileData.Items;
    SampleMeshHeader* meshHeader = (SampleMeshHeader*)fileDataPointer;

    if (meshHeader->FileId[0] == 'M' && meshHeader->FileId[1] == 'E' && meshHeader->FileId[2] == 'S' && meshHeader->FileId[3] == 'H')
    {
        printf("OK Meshlet Count: %d\n", meshHeader->MeshletCount);
    }

    meshData->MeshletCount = meshHeader->MeshletCount;

    CreateAndUploadDataTemp(&meshData->VertexBuffer, &meshData->VertexBufferReadDescriptor, applicationPayload, fileDataPointer + meshHeader->VertexBufferOffset, meshHeader->VertexBufferSizeInBytes);
    CreateAndUploadDataTemp(&meshData->MeshletBuffer, &meshData->MeshletBufferReadDescriptor, applicationPayload, fileDataPointer + meshHeader->MeshletBufferOffset, meshHeader->MeshletBufferSizeInBytes);
    CreateAndUploadDataTemp(&meshData->MeshletVertexIndexBuffer, &meshData->MeshletVertexIndexBufferReadDescriptor, applicationPayload, fileDataPointer + meshHeader->MeshletVertexIndexBufferOffset, meshHeader->MeshletVertexIndexBufferSizeInBytes);
    CreateAndUploadDataTemp(&meshData->MeshletTriangleIndexBuffer, &meshData->MeshletTriangleIndexBufferReadDescriptor, applicationPayload, fileDataPointer + meshHeader->MeshletTriangleIndexBufferOffset, meshHeader->MeshletTriangleIndexBufferSizeInBytes);

    applicationPayload->ShaderParameters.VertexBuffer = meshData->VertexBufferReadDescriptor;
    applicationPayload->ShaderParameters.MeshletBuffer = meshData->MeshletBufferReadDescriptor;
    applicationPayload->ShaderParameters.MeshletVertexIndexBuffer = meshData->MeshletVertexIndexBufferReadDescriptor;
    applicationPayload->ShaderParameters.MeshletTriangleIndexBuffer = meshData->MeshletTriangleIndexBufferReadDescriptor;
    applicationPayload->ShaderParameters.MeshletCount = meshData->MeshletCount;
}

void FreeMesh(MeshData* meshData)
{
    ElemFreeGraphicsResourceDescriptor(meshData->VertexBufferReadDescriptor, NULL);
    ElemFreeGraphicsResource(meshData->VertexBuffer, NULL);
    ElemFreeGraphicsResourceDescriptor(meshData->MeshletBufferReadDescriptor, NULL);
    ElemFreeGraphicsResource(meshData->MeshletBuffer, NULL);
    ElemFreeGraphicsResourceDescriptor(meshData->MeshletVertexIndexBufferReadDescriptor, NULL);
    ElemFreeGraphicsResource(meshData->MeshletVertexIndexBuffer, NULL);
    ElemFreeGraphicsResourceDescriptor(meshData->MeshletTriangleIndexBufferReadDescriptor, NULL);
    ElemFreeGraphicsResource(meshData->MeshletTriangleIndexBuffer, NULL);
}

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->Window = ElemCreateWindow(&(ElemWindowOptions) { .WindowState = applicationPayload->AppSettings.PreferFullScreen ? ElemWindowState_FullScreen : ElemWindowState_Normal });

    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = true, .EnableGpuValidation = false, .EnableDebugBarrierInfo = false, .PreferVulkan = applicationPayload->AppSettings.PreferVulkan });
    
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

    // TODO: For now we need to put the heap as GpuUpload but it should be Gpu when we use IOQueues
    // TODO: Having GPU Upload is still annoying 😞
    applicationPayload->GraphicsHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_GpuUpload });

    CreateDepthBuffer(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);
    LoadMesh(&applicationPayload->TestMeshData, "kitten.scene", applicationPayload);
    //LoadMesh(&applicationPayload->TestMeshData, "buddha.scene", applicationPayload);

    ElemDataSpan shaderData = SampleReadFile(!applicationPayload->AppSettings.PreferVulkan ? "RenderMesh.shader": "RenderMesh_vulkan.shader");
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

    applicationPayload->ShaderParameters.RotationQuaternion = (SampleVector4){ .X = 0, .Y = 0, .Z = 0, .W = 1 };

    SampleInputsApplicationInit(&applicationPayload->InputsApplication);
    SampleInputsModelViewerInit(&applicationPayload->InputsModelViewer);
    
    if (applicationPayload->AppSettings.PreferFullScreen)
    {
        ElemHideWindowCursor(applicationPayload->Window);
        applicationPayload->InputsApplication.State.IsCursorDisplayed = false;
    }

    SampleStartFrameMeasurement();
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemWaitForFenceOnCpu(applicationPayload->LastExecutionFence);

    FreeMesh(&applicationPayload->TestMeshData);

    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
 
    ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    ElemFreeGraphicsHeap(applicationPayload->DepthBufferHeap);

    ElemFreeGraphicsHeap(applicationPayload->GraphicsHeap);
    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);
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
    SampleInputsModelViewerUpdate(inputStream, &applicationPayload->InputsModelViewer, updateParameters->DeltaTimeInSeconds);

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

    SampleInputsModelViewerState* modelViewerState = &applicationPayload->InputsModelViewer.State;

    if (SampleMagnitudeSquaredV3(modelViewerState->RotationDelta))
    {
        SampleVector4 rotationQuaternion = SampleMulQuat(SampleCreateQuaternion((SampleVector3){ 1, 0, 0 }, modelViewerState->RotationDelta.X), 
                                                         SampleMulQuat(SampleCreateQuaternion((SampleVector3){ 0, 0, 1 }, modelViewerState->RotationDelta.Z),
                                                                       SampleCreateQuaternion((SampleVector3){ 0, 1, 0 }, modelViewerState->RotationDelta.Y)));

        applicationPayload->ShaderParameters.RotationQuaternion = SampleMulQuat(rotationQuaternion, applicationPayload->ShaderParameters.RotationQuaternion);
    }

    applicationPayload->ShaderParameters.AspectRatio = updateParameters->SwapChainInfo.AspectRatio;
    float maxZoom = (applicationPayload->ShaderParameters.AspectRatio >= 0.75 ? 1.5f : 3.5f);
    applicationPayload->ShaderParameters.Zoom = fminf(maxZoom, modelViewerState->Zoom);
    applicationPayload->ShaderParameters.ShowMeshlets = modelViewerState->Action;

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
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });
    ElemDispatchMesh(commandList, applicationPayload->TestMeshData.MeshletCount, 1, 1);

    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);
    applicationPayload->LastExecutionFence = ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, NULL);

    ElemPresentSwapChain(applicationPayload->SwapChain);
    SampleFrameMeasurement frameMeasurement = SampleEndFrameMeasurement();

    if (frameMeasurement.HasNewData)
    {
        SampleSetWindowTitle(applicationPayload->Window, "HelloMesh", applicationPayload->GraphicsDevice, frameMeasurement.FrameTimeInSeconds, frameMeasurement.Fps);
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
        .ApplicationName = "Hello Mesh",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });
}
