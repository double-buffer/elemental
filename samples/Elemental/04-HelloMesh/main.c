#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"
#include "SampleInputsApplication.h"
#include "SampleInputsModelViewer.h"

#define MESH_FILE_VERSION 1u
#define MESH_VERTEX_SIZE_IN_BYTES (sizeof(float) * 12u)
#define MESHLET_SIZE_IN_BYTES (sizeof(uint32_t) * 4u)

typedef struct
{
    char FileId[4];
    uint32_t Version;
    uint32_t MeshBufferSizeInBytes;
    uint32_t VertexSizeInBytes;
    uint32_t VertexBufferOffset;
    uint32_t MeshletOffset;
    uint32_t MeshletVertexIndexOffset;
    uint32_t MeshletTriangleIndexOffset;
    uint32_t MeshletCount;
} MeshFileHeader;

typedef struct
{
    uint32_t MeshBuffer;
    uint32_t VertexBufferOffset;
    uint32_t MeshletOffset;
    uint32_t MeshletVertexIndexOffset;
    uint32_t MeshletTriangleIndexOffset;
    uint32_t Reserved1;
    uint32_t Reserved2;
    uint32_t Reserved3;
    SampleVector4 RotationQuaternion;
    float Zoom;
    float AspectRatio;
    uint32_t ShowMeshlets;
    uint32_t MeshletCount;
} ShaderParameters;

typedef struct
{
    SampleAppSettings AppSettings;
    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    ElemFence LastExecutionFence;
    ElemSwapChain SwapChain;
    ElemGraphicsHeap DepthBufferHeap;
    ElemGraphicsResource DepthBuffer;
    ElemGraphicsHeap MeshBufferHeap;
    ElemGraphicsResource MeshBuffer;
    ElemGraphicsResourceDescriptor MeshBufferReadDescriptor;
    ElemPipelineState GraphicsPipeline;
    ShaderParameters ShaderParameters;
    SampleInputsApplication InputsApplication;
    SampleInputsModelViewer InputsModelViewer;
} ApplicationPayload;

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

bool LoadMesh(ApplicationPayload* applicationPayload, const char* path)
{
    ElemDataSpan meshFileData = SampleReadFile(path, true);

    if (!meshFileData.Items || meshFileData.Length < sizeof(MeshFileHeader))
    {
        printf("Unable to read mesh: %s\n", path);
        return false;
    }

    MeshFileHeader* header = (MeshFileHeader*)meshFileData.Items;
    uint32_t meshPayloadSizeInBytes = meshFileData.Length - sizeof(MeshFileHeader);

    bool validHeader =
        memcmp(header->FileId, "MESH", 4) == 0 &&
        header->Version == MESH_FILE_VERSION &&
        header->MeshBufferSizeInBytes == meshPayloadSizeInBytes &&
        header->VertexSizeInBytes == MESH_VERTEX_SIZE_IN_BYTES &&
        header->VertexBufferOffset == 0 &&
        header->MeshletOffset <= header->MeshletVertexIndexOffset &&
        header->MeshletVertexIndexOffset <= header->MeshletTriangleIndexOffset &&
        header->MeshletTriangleIndexOffset <= header->MeshBufferSizeInBytes &&
        header->MeshletCount > 0 &&
        header->MeshletOffset + header->MeshletCount * MESHLET_SIZE_IN_BYTES <= header->MeshletVertexIndexOffset;

    if (!validHeader)
    {
        printf("Invalid or unsupported mesh file: %s\n", path);
        free(meshFileData.Items);
        return false;
    }

    ElemGraphicsResourceInfo meshBufferInfo = ElemCreateGraphicsBufferResourceInfo(
        applicationPayload->GraphicsDevice,
        header->MeshBufferSizeInBytes,
        ElemGraphicsResourceUsage_Read,
        &(ElemGraphicsResourceInfoOptions) { .DebugName = "MeshBuffer" });

    applicationPayload->MeshBufferHeap = ElemCreateGraphicsHeap(
        applicationPayload->GraphicsDevice,
        meshBufferInfo.SizeInBytes,
        &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_GpuUpload });

    applicationPayload->MeshBuffer = ElemCreateGraphicsResource(applicationPayload->MeshBufferHeap, 0, &meshBufferInfo);
    applicationPayload->MeshBufferReadDescriptor = ElemCreateGraphicsResourceDescriptor(
        applicationPayload->MeshBuffer,
        ElemGraphicsResourceDescriptorUsage_Read,
        NULL);

    ElemUploadGraphicsBufferData(
        applicationPayload->MeshBuffer,
        0,
        (ElemDataSpan)
        {
            .Items = meshFileData.Items + sizeof(MeshFileHeader),
            .Length = header->MeshBufferSizeInBytes
        });

    applicationPayload->ShaderParameters.MeshBuffer = applicationPayload->MeshBufferReadDescriptor;
    applicationPayload->ShaderParameters.VertexBufferOffset = header->VertexBufferOffset;
    applicationPayload->ShaderParameters.MeshletOffset = header->MeshletOffset;
    applicationPayload->ShaderParameters.MeshletVertexIndexOffset = header->MeshletVertexIndexOffset;
    applicationPayload->ShaderParameters.MeshletTriangleIndexOffset = header->MeshletTriangleIndexOffset;
    applicationPayload->ShaderParameters.MeshletCount = header->MeshletCount;

    free(meshFileData.Items);
    return true;
}

void CreateDepthBuffer(ApplicationPayload* applicationPayload, uint32_t width, uint32_t height)
{
    if (applicationPayload->DepthBuffer != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    }

    ElemGraphicsResourceInfo resourceInfo = ElemCreateTexture2DResourceInfo(
        applicationPayload->GraphicsDevice,
        width,
        height,
        1,
        ElemGraphicsFormat_D32_FLOAT,
        ElemGraphicsResourceUsage_DepthStencil,
        &(ElemGraphicsResourceInfoOptions) { .DebugName = "DepthBuffer" });

    applicationPayload->DepthBuffer = ElemCreateGraphicsResource(applicationPayload->DepthBufferHeap, 0, &resourceInfo);
}

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->Window = ElemCreateWindow(&(ElemWindowOptions) { .WindowState = applicationPayload->AppSettings.PreferFullScreen ? ElemWindowState_FullScreen : ElemWindowState_Normal });

    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = true, .EnableGpuValidation = false, .EnableDebugBarrierInfo = false, .PreferVulkan = applicationPayload->AppSettings.PreferVulkan });

    applicationPayload->GraphicsDevice = ElemCreateGraphicsDevice(NULL);
    applicationPayload->CommandQueue = ElemCreateCommandQueue(applicationPayload->GraphicsDevice, ElemCommandQueueType_Graphics, NULL);
    applicationPayload->SwapChain = ElemCreateSwapChain(applicationPayload->CommandQueue, applicationPayload->Window, UpdateSwapChain, &(ElemSwapChainOptions) { .FrameLatency = 1, .UpdatePayload = payload });
    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(applicationPayload->SwapChain);

    applicationPayload->DepthBufferHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_Gpu });
    CreateDepthBuffer(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);

    if (!LoadMesh(applicationPayload, "kitten.mesh"))
    {
        ElemExitApplication(1);
        return;
    }

    ElemDataSpan shaderData = SampleReadFile(!applicationPayload->AppSettings.PreferVulkan ? "RenderMesh.shader" : "RenderMesh_vulkan.shader", true);
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

    free(shaderData.Items);
    ElemFreeShaderLibrary(shaderLibrary);

    applicationPayload->ShaderParameters.RotationQuaternion = (SampleVector4) { .X = 0, .Y = 0, .Z = 0, .W = 1 };

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

    if (applicationPayload->LastExecutionFence.CommandQueue != ELEM_HANDLE_NULL)
    {
        ElemWaitForFenceOnCpu(applicationPayload->LastExecutionFence);
    }

    if (applicationPayload->GraphicsPipeline != ELEM_HANDLE_NULL)
    {
        ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    }

    if (applicationPayload->MeshBufferReadDescriptor != -1)
    {
        ElemFreeGraphicsResourceDescriptor(applicationPayload->MeshBufferReadDescriptor, NULL);
    }

    if (applicationPayload->MeshBuffer != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsResource(applicationPayload->MeshBuffer, NULL);
    }

    if (applicationPayload->MeshBufferHeap != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsHeap(applicationPayload->MeshBufferHeap);
    }

    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);

    if (applicationPayload->DepthBuffer != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    }

    if (applicationPayload->DepthBufferHeap != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsHeap(applicationPayload->DepthBufferHeap);
    }

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
        ElemShowWindowCursor(applicationPayload->Window);
    }
    else if (applicationPayload->InputsApplication.State.HideCursor)
    {
        ElemHideWindowCursor(applicationPayload->Window);
    }

    SampleInputsModelViewerState* modelViewerState = &applicationPayload->InputsModelViewer.State;

    if (SampleMagnitudeSquaredV3(modelViewerState->RotationDelta))
    {
        SampleVector4 rotationQuaternion = SampleMulQuat(
            SampleCreateQuaternion((ElemVector3) { 1, 0, 0 }, modelViewerState->RotationDelta.X),
            SampleMulQuat(
                SampleCreateQuaternion((ElemVector3) { 0, 0, 1 }, modelViewerState->RotationDelta.Z),
                SampleCreateQuaternion((ElemVector3) { 0, 1, 0 }, modelViewerState->RotationDelta.Y)));

        applicationPayload->ShaderParameters.RotationQuaternion = SampleMulQuat(rotationQuaternion, applicationPayload->ShaderParameters.RotationQuaternion);
    }

    applicationPayload->ShaderParameters.AspectRatio = updateParameters->SwapChainInfo.AspectRatio;
    float maxZoom = applicationPayload->ShaderParameters.AspectRatio >= 0.75 ? 1.5f : 3.5f;
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
    ElemDispatchMesh(commandList, applicationPayload->ShaderParameters.MeshletCount, 1, 1);

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
        .AppSettings = SampleParseAppSettings(argc, argv),
        .MeshBufferReadDescriptor = -1
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
