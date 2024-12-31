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

    // TODO: Dynamically manage heaps and buffers
    CreateBuffer(&imGuiData->VertexBuffer, &imGuiData->VertexBufferReadDescriptor, payload, 20000 * sizeof(ImDrawVert));
    CreateBuffer(&imGuiData->IndexBuffer, &imGuiData->IndexBufferReadDescriptor, payload, 20000 * sizeof(ImDrawIdx));

    // TODO: Font 
    uint8_t* fontPixels;
    int32_t fontWidth;
    int32_t fontHeight;
    int32_t fontBytesPerPixel;
    ImFontAtlas_GetTexDataAsRGBA32(imGuiIO->Fonts, &fontPixels, &fontWidth, &fontHeight, &fontBytesPerPixel);

    payload->ImGuiBackendData.FontTextureId = 1;
    imGuiIO->Fonts->TexID = (ImTextureID)&imGuiData->FontTextureId;

    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(payload->SwapChain);

    ElemDataSpan shaderData = SampleReadFile(!payload->PreferVulkan ? "RenderImGui.shader": "RenderImGui_vulkan.shader", true);
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(payload->GraphicsDevice, shaderData);

    imGuiData->RenderPipeline = ElemCompileGraphicsPipelineState(payload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "RenderImGui PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .CullMode = ElemGraphicsCullMode_None,
        .RenderTargets = 
        { 
            .Items = (ElemGraphicsPipelineStateRenderTarget[]) {
            { 
                .Format = swapChainInfo.Format,
                .BlendOperation = ElemGraphicsBlendOperation_Add,
                .SourceBlendFactor = ElemGraphicsBlendFactor_SourceAlpha,
                .DestinationBlendFactor = ElemGraphicsBlendFactor_InverseSourceAlpha,
                .BlendOperationAlpha = ElemGraphicsBlendOperation_Add,
                .SourceBlendFactorAlpha = ElemGraphicsBlendFactor_One,
                .DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_InverseSourceAlpha
            }}, 
            .Length = 1 
        },
    });

    ElemFreeShaderLibrary(shaderLibrary);
}

void ImGuidElementalNewFrame(const ElemSwapChainUpdateParameters* updateParameters, const ElemInputStream inputStream)
{
    // TODO: Investigate: https://github.com/ocornut/imgui/releases/tag/v1.91.1
    //ImGuiPlatformIO* imGuiPlatformIO = igGetPlatformIO();

    ImGuiIO* imGuiIO = igGetIO();

    imGuiIO->DisplaySize = (ImVec2)
    { 
        (float)updateParameters->SwapChainInfo.Width / updateParameters->SwapChainInfo.UIScale, 
        (float)updateParameters->SwapChainInfo.Height / updateParameters->SwapChainInfo.UIScale 
    };

    imGuiIO->DisplayFramebufferScale = (ImVec2) { updateParameters->SwapChainInfo.UIScale, updateParameters->SwapChainInfo.UIScale };
    imGuiIO->DeltaTime = updateParameters->DeltaTimeInSeconds;

    for (uint32_t i = 0; i < inputStream.Events.Length; i++)
    {
        ElemInputEvent* inputEvent = &inputStream.Events.Items[i];

        if (inputEvent->InputId == ElemInputId_MouseAxisXNegative ||
            inputEvent->InputId == ElemInputId_MouseAxisXPositive ||
            inputEvent->InputId == ElemInputId_MouseAxisYNegative ||
            inputEvent->InputId == ElemInputId_MouseAxisYPositive)
        {
            ElemWindowCursorPosition cursorPosition = ElemGetWindowCursorPosition(updateParameters->SwapChainInfo.Window);
            ImGuiIO_AddMousePosEvent(imGuiIO, cursorPosition.X / updateParameters->SwapChainInfo.UIScale, cursorPosition.Y / updateParameters->SwapChainInfo.UIScale); 
        }

        if (inputEvent->InputId == ElemInputId_MouseLeftButton)
        {
            ImGuiIO_AddMouseButtonEvent(imGuiIO, ImGuiMouseButton_Left, inputEvent->Value);
        }

        if (inputEvent->InputId == ElemInputId_MouseWheelNegative || inputEvent->InputId == ElemInputId_MouseWheelPositive)
        {
            // TODO: Input wheel should maybe normalized to one?
            ImGuiIO_AddMouseWheelEvent(imGuiIO, 0.0f, (inputEvent->InputId == ElemInputId_MouseWheelPositive ? inputEvent->Value : -inputEvent->Value) / 10.0f);
        }
    }

    //imGuiIO->ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts | ImGuiConfigFlags_DpiEnableScaleViewports;
}

void ImGuiRenderDrawData(ImDrawData* drawData, ElemCommandList commandList, ApplicationPayload* payload, const ElemSwapChainUpdateParameters* updateParameters)
{
    ElemImGuiBackendData* imGuiData = &payload->ImGuiBackendData;

    uint32_t currentVertexOffset = 0;
    uint32_t currentIndexOffset = 0;

    for (int32_t i = 0; i < drawData->CmdListsCount; i++)
    {
        ImDrawList* imCommandList = drawData->CmdLists.Data[i];

        for (int32_t j = 0; j < imCommandList->CmdBuffer.Size; j++)
        {
            ImDrawCmd* drawCommand = &imCommandList->CmdBuffer.Data[j];

            ImGuiShaderParameters shaderParameters =
            {
                .VertexBufferIndex = imGuiData->VertexBufferReadDescriptor,
                .IndexBufferIndex = imGuiData->IndexBufferReadDescriptor,
                .VertexOffset = currentVertexOffset + drawCommand->VtxOffset,
                .IndexOffset = currentIndexOffset + drawCommand->IdxOffset,
                .IndexCount = drawCommand->ElemCount,
                .RenderWidth = drawData->DisplaySize.x,
                .RenderHeight = drawData->DisplaySize.y
            };

            ElemBindPipelineState(commandList, imGuiData->RenderPipeline); 
            ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&shaderParameters, .Length = sizeof(ImGuiShaderParameters) });
            ElemSetScissorRectangle(commandList, &(ElemRectangle) { 
                .X = drawCommand->ClipRect.x * updateParameters->SwapChainInfo.UIScale,
                .Y = drawCommand->ClipRect.y * updateParameters->SwapChainInfo.UIScale,
                .Width = (drawCommand->ClipRect.z - drawCommand->ClipRect.x) * updateParameters->SwapChainInfo.UIScale,
                .Height = (drawCommand->ClipRect.w - drawCommand->ClipRect.y) * updateParameters->SwapChainInfo.UIScale
            });

            uint32_t threadSize = 126;
            uint32_t dispatchCount = (drawCommand->ElemCount + (threadSize - 1)) / threadSize;
            ElemDispatchMesh(commandList, dispatchCount, 1, 1);
        }

        ElemUploadGraphicsBufferData(imGuiData->VertexBuffer, currentVertexOffset * sizeof(ImDrawVert), (ElemDataSpan) { 
            .Items = (uint8_t*)imCommandList->VtxBuffer.Data, 
            .Length = imCommandList->VtxBuffer.Size * sizeof(ImDrawVert) 
        });

        currentVertexOffset += imCommandList->VtxBuffer.Size;

        ElemUploadGraphicsBufferData(imGuiData->IndexBuffer, currentIndexOffset * sizeof(ImDrawIdx), (ElemDataSpan) { 
            .Items = (uint8_t*)imCommandList->IdxBuffer.Data, 
            .Length = imCommandList->IdxBuffer.Size * sizeof(ImDrawIdx) 
        });

        currentIndexOffset += imCommandList->IdxBuffer.Size;
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
    
    ElemInputStream inputStream = ElemGetInputStream();

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

    ImGuidElementalNewFrame(updateParameters, inputStream);
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
