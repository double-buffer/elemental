#include "Elemental.h"
#include "../Common/SampleUtils.h"

typedef struct
{
    float AspectRatio;
    float RotationY;
} ShaderParameters;

typedef struct
{
    const char* ProgramPath;
    bool PreferVulkan;
    ElemWindow Window;
    ElemWindowSize CurrentRenderSize;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    ElemSwapChain SwapChain;
    ElemPipelineState GraphicsPipeline;
    ShaderParameters ShaderParameters;
} ApplicationPayload;

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemWindow window = ElemCreateWindow(NULL);
    ElemWindowSize currentRenderSize = ElemGetWindowRenderSize(window);

    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = true, .PreferVulkan = applicationPayload->PreferVulkan });
    ElemGraphicsDevice graphicsDevice = ElemCreateGraphicsDevice(NULL);

    ElemCommandQueue commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, &(ElemCommandQueueOptions) { .DebugName = "TestCommandQueue" });
    ElemSwapChain swapChain = ElemCreateSwapChain(commandQueue, window, UpdateSwapChain, &(ElemSwapChainOptions) { .UpdatePayload = payload });
    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(swapChain);

    ElemDataSpan shaderData = SampleReadFile(applicationPayload->ProgramPath, "Triangle.shader");
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(graphicsDevice, (ElemDataSpan) { .Items = shaderData.Items, .Length = shaderData.Length });

    ElemPipelineState graphicsPipeline = ElemCompileGraphicsPipelineState(graphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "Test PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .TextureFormats = { .Items = (ElemTextureFormat[]) { swapChainInfo.Format }, .Length = 1 }
    });
    
    ElemFreeShaderLibrary(shaderLibrary);

    applicationPayload->Window = window;
    applicationPayload->CurrentRenderSize = currentRenderSize;
    applicationPayload->GraphicsDevice = graphicsDevice;
    applicationPayload->CommandQueue = commandQueue;
    applicationPayload->SwapChain = swapChain;
    applicationPayload->GraphicsPipeline = graphicsPipeline;
    
    SampleStartFrameMeasurement();
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);

    printf("Exit Sample\n");
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemWindowSize renderSize = ElemGetWindowRenderSize(applicationPayload->Window);

    if (renderSize.Width != applicationPayload->CurrentRenderSize.Width || renderSize.Height != applicationPayload->CurrentRenderSize.Height)
    {
        ElemResizeSwapChain(applicationPayload->SwapChain, renderSize.Width, renderSize.Height);
        applicationPayload->CurrentRenderSize = renderSize;
    }

    ElemCommandList commandList = ElemGetCommandList(applicationPayload->CommandQueue, &(ElemCommandListOptions) { .DebugName = "TestCommandList" }); 

    ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters) {
        .RenderTargets = 
        {
            .Items = (ElemRenderPassRenderTarget[]){ 
            {
                .RenderTarget = updateParameters->BackBufferTexture, 
                .ClearColor = { 0.0f, 0.01f, 0.02f, 1.0f },
                .LoadAction = ElemRenderPassLoadAction_Clear
            }},
            .Length = 1
        }
    });

    ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline);
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });
    ElemDispatchMesh(commandList, 1, 1, 1);

    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);
    ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, NULL);

    ElemPresentSwapChain(applicationPayload->SwapChain);

    SampleFrameMeasurement frameMeasurement = SampleEndFrameMeasurement();

    ElemGraphicsDeviceInfo graphicsDeviceInfo = ElemGetGraphicsDeviceInfo(applicationPayload->GraphicsDevice);
    SampleSetWindowTitle(applicationPayload->Window, "HelloTriangle", graphicsDeviceInfo, frameMeasurement.FrameTime, frameMeasurement.Fps);

    applicationPayload->ShaderParameters.AspectRatio = (float)renderSize.Width / renderSize.Height;
    applicationPayload->ShaderParameters.RotationY += 1.5f * updateParameters->DeltaTimeInSeconds;
    
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
        .ProgramPath = argv[0],
        .PreferVulkan = preferVulkan
    };

    ElemRunApplication(&(ElemRunApplicationParameters)
    {
        .ApplicationName = "Hello Triangle",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });
}

