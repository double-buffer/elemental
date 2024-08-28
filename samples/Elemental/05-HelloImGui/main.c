#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include "Elemental.h"
#include "SampleUtils.h"

typedef struct
{
    uint32_t FontTextureId;
    ElemGraphicsResource VertexBuffer;
    ElemGraphicsResourceDescriptor VertexBufferReadDescriptor;
    ElemGraphicsResource IndexBuffer;
    ElemGraphicsResourceDescriptor IndexBufferReadDescriptor;
    ElemPipelineState RenderPipeline;
} ElemImGuiBackendData;

typedef struct 
{
    uint32_t VertexBufferIndex;
    uint32_t IndexBufferIndex;
    uint32_t VertexOffset;
    uint32_t VertexCount;
    uint32_t IndexOffset;
    uint32_t IndexCount;
    uint32_t RenderWidth;
    uint32_t RenderHeight;
} ImGuiShaderParameters;

typedef struct
{
    bool PreferVulkan;
    bool ShowImGuiDemoWindow;
    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    ElemGraphicsHeap GraphicsHeap;
    uint32_t CurrentHeapOffset;
    ElemFence LastExecutionFence;
    ElemSwapChain SwapChain;
    ElemImGuiBackendData ImGuiBackendData; 
} ApplicationPayload;
    
void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

void CreateBuffer(ElemGraphicsResource* buffer, ElemGraphicsResourceDescriptor* readDescriptor, ApplicationPayload* applicationPayload, uint32_t sizeInBytes)
{
    // TODO: Alignment should be used with the offset before adding the size of the resource!
    ElemGraphicsResourceInfo bufferDescription = ElemCreateGraphicsBufferResourceInfo(applicationPayload->GraphicsDevice, sizeInBytes, ElemGraphicsResourceUsage_Read, NULL);

    applicationPayload->CurrentHeapOffset = SampleAlignValue(applicationPayload->CurrentHeapOffset, bufferDescription.Alignment);
    *buffer = ElemCreateGraphicsResource(applicationPayload->GraphicsHeap, applicationPayload->CurrentHeapOffset, &bufferDescription);
    applicationPayload->CurrentHeapOffset += bufferDescription.SizeInBytes;

    *readDescriptor = ElemCreateGraphicsResourceDescriptor(*buffer, ElemGraphicsResourceDescriptorUsage_Read, NULL);
}

void ImGuiInitBackend(ApplicationPayload* payload)
{
    ElemImGuiBackendData* imGuiData = &payload->ImGuiBackendData;

    ImGuiIO* imGuiIO = igGetIO();
    imGuiIO->BackendRendererUserData = &payload->ImGuiBackendData;
    imGuiIO->BackendRendererName = "Elemental";
    imGuiIO->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    //imGuiIO->ConfigFlags |= ImGuiConfigFlags_IsSRGB;

    CreateBuffer(&imGuiData->VertexBuffer, &imGuiData->VertexBufferReadDescriptor, payload, 5000 * sizeof(ImDrawVert));
    CreateBuffer(&imGuiData->IndexBuffer, &imGuiData->IndexBufferReadDescriptor, payload, 5000 * sizeof(ImDrawIdx));

    // TODO: Font 
    uint8_t* fontPixels;
    uint32_t fontWidth;
    uint32_t fontHeight;
    uint32_t fontBytesPerPixel;
    ImFontAtlas_GetTexDataAsRGBA32(imGuiIO->Fonts, &fontPixels, &fontWidth, &fontHeight, &fontBytesPerPixel);

    payload->ImGuiBackendData.FontTextureId = 1;
    imGuiIO->Fonts->TexID = (void*)&imGuiData->FontTextureId;

    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(payload->SwapChain);

    ElemDataSpan shaderData = SampleReadFile(!payload->PreferVulkan ? "RenderImGui.shader": "RenderImGui_vulkan.shader");
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(payload->GraphicsDevice, shaderData);

    imGuiData->RenderPipeline = ElemCompileGraphicsPipelineState(payload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "RenderImGui PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .RenderTargets = 
        { 
            .Items = (ElemGraphicsPipelineStateRenderTarget[]) {
            { 
                .Format = swapChainInfo.Format,
                //.BlendOperation = ElemGraphicsBlendOperation_Add,
                //.SourceBlendFactor = ElemGraphicsBlendFactor_One,
                //.DestinationBlendFactor = ElemGraphicsBlendFactor_InverseSourceAlpha,
                //.BlendOperationAlpha = ElemGraphicsBlendOperation_Add,
                //.SourceBlendFactorAlpha = ElemGraphicsBlendFactor_One,
                //.DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_InverseSourceAlpha
            }}, 
            .Length = 1 
        },
    });

    ElemFreeShaderLibrary(shaderLibrary);
}

void ImGuidElementalNewFrame(const ElemSwapChainUpdateParameters* updateParameters)
{
    ImGuiIO* imGuiIO = igGetIO();

    // TODO: Set Correct UI Scale
    imGuiIO->DisplaySize = (ImVec2){ (float)updateParameters->SwapChainInfo.Width / 2.0f, (float)updateParameters->SwapChainInfo.Height / 2.0f };
    imGuiIO->DisplayFramebufferScale = (ImVec2) { 2.0f, 2.0f };

    //imGuiIO->DisplaySize = (ImVec2){ updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height };
    //imGuiIO->ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts | ImGuiConfigFlags_DpiEnableScaleViewports;
}

void ImGuiRenderDrawData(ImDrawData* drawData, ElemCommandList commandList, ApplicationPayload* payload, const ElemSwapChainUpdateParameters* updateParameters)
{
    ElemImGuiBackendData* imGuiData = &payload->ImGuiBackendData;

    printf("DrawData VertexCount=%d, IndexCount=%d, CommandList=%d\n", drawData->TotalVtxCount, drawData->TotalIdxCount , drawData->CmdListsCount);

    uint32_t currentVertexOffset = 0;
    ElemDataSpan vertexBufferPointer = ElemGetGraphicsResourceDataSpan(imGuiData->VertexBuffer);

    uint32_t currentIndexOffset = 0;
    ElemDataSpan indexBufferPointer = ElemGetGraphicsResourceDataSpan(imGuiData->IndexBuffer);

    for (int32_t i = 0; i < drawData->CmdListsCount; i++)
    {
        ImDrawList* imCommandList = drawData->CmdLists.Data[i];

        ImGuiShaderParameters shaderParameters =
        {
            .VertexBufferIndex = imGuiData->VertexBufferReadDescriptor,
            .IndexBufferIndex = imGuiData->IndexBufferReadDescriptor,
            .VertexOffset = currentVertexOffset,
            .VertexCount = imCommandList->VtxBuffer.Size,
            .IndexOffset = currentIndexOffset,
            .IndexCount = imCommandList->IdxBuffer.Size,
            .RenderWidth = drawData->DisplaySize.x,
            .RenderHeight = drawData->DisplaySize.y
        };

        memcpy(vertexBufferPointer.Items + currentVertexOffset, imCommandList->VtxBuffer.Data, imCommandList->VtxBuffer.Size * sizeof(ImDrawVert));
        currentVertexOffset += imCommandList->VtxBuffer.Size;

        memcpy(indexBufferPointer.Items + currentIndexOffset, imCommandList->IdxBuffer.Data, imCommandList->IdxBuffer.Size * sizeof(ImDrawIdx));
        currentIndexOffset += imCommandList->IdxBuffer.Size;

        ElemBindPipelineState(commandList, imGuiData->RenderPipeline); 
        ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&shaderParameters, .Length = sizeof(ImGuiShaderParameters) });
    
        uint32_t threadSize = 128;
        //ElemDispatchMesh(commandList, (imCommandList->IdxBuffer.Size / 3 + (threadSize - 1)) / threadSize, 1, 1);
        ElemDispatchMesh(commandList, 10, 1, 1);
    }
}

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->Window = ElemCreateWindow(NULL);

    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = true, .EnableGpuValidation = false, .EnableDebugBarrierInfo = false, .PreferVulkan = applicationPayload->PreferVulkan });
    applicationPayload->GraphicsDevice = ElemCreateGraphicsDevice(NULL);

    applicationPayload->CommandQueue= ElemCreateCommandQueue(applicationPayload->GraphicsDevice, ElemCommandQueueType_Graphics, NULL);
    applicationPayload->SwapChain= ElemCreateSwapChain(applicationPayload->CommandQueue, applicationPayload->Window, UpdateSwapChain, &(ElemSwapChainOptions) { .FrameLatency = 1, .UpdatePayload = payload });

    applicationPayload->GraphicsHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_GpuUpload });

    SampleStartFrameMeasurement();

    igCreateContext(NULL);
    ImGuiInitBackend(applicationPayload);
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemWaitForFenceOnCpu(applicationPayload->LastExecutionFence);

    ElemFreePipelineState(applicationPayload->ImGuiBackendData.RenderPipeline);
    ElemFreeGraphicsResource(applicationPayload->ImGuiBackendData.VertexBuffer, NULL);
    ElemFreeGraphicsResourceDescriptor(applicationPayload->ImGuiBackendData.VertexBufferReadDescriptor, NULL);
    ElemFreeGraphicsResource(applicationPayload->ImGuiBackendData.IndexBuffer, NULL);
    ElemFreeGraphicsResourceDescriptor(applicationPayload->ImGuiBackendData.IndexBufferReadDescriptor, NULL);

    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
 
    ElemFreeGraphicsHeap(applicationPayload->GraphicsHeap);
    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

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
        }
    });

    ImGuidElementalNewFrame(updateParameters);
    igNewFrame();
    igShowDemoWindow(&applicationPayload->ShowImGuiDemoWindow);
    igRender();

    ImGuiRenderDrawData(igGetDrawData(), commandList, applicationPayload, updateParameters);

    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);
    applicationPayload->LastExecutionFence = ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, NULL);

    ElemPresentSwapChain(applicationPayload->SwapChain);
    SampleFrameMeasurement frameMeasurement = SampleEndFrameMeasurement();

    if (frameMeasurement.HasNewData)
    {
        SampleSetWindowTitle(applicationPayload->Window, "HelloImGui", applicationPayload->GraphicsDevice, frameMeasurement.FrameTimeInSeconds, frameMeasurement.Fps);
    }
    
    SampleStartFrameMeasurement();
}

int main(int argc, const char* argv[]) 
{
    bool preferVulkan = false;

    if (argc > 1 && strcmp(argv[1], "--vulkan") == 0)
    {
        preferVulkan = true;
    }

    ElemConfigureLogHandler(ElemConsoleLogHandler);

    ApplicationPayload payload =
    {
        .PreferVulkan = preferVulkan
    };

    ElemRunApplication(&(ElemRunApplicationParameters)
    {
        .ApplicationName = "Hello ImGui",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });
}
