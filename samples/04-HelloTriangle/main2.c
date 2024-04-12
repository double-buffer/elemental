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

static ShaderParameters globalShaderParameters;

void UpdateSwapChain(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    SampleStartFrameMeasurement();

    // TODO: Remove that? Not sure because maybe we want to have 2 phases, one for inputs and one for rendering
    ElemWaitForSwapChainOnCpu(applicationPayload->SwapChain);

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
                    ElemGetSwapChainBackBufferTexture(applicationPayload->SwapChain),
                    .ClearColor = { 0.0f, 0.01f, 0.02f, 1.0f },
                    .LoadAction = ElemRenderPassLoadAction_Clear
                }
            },
            .Length = 1
        }
    });

    ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline);
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&globalShaderParameters, .Length = sizeof(ShaderParameters) });
    ElemDispatchMesh(commandList, 1, 1, 1);

    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);
    ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, NULL);

    // NOTE: Present needs to be called by the client code because with it you can still do cpu work after present
    // For work before an available swap chain, just increase latency?
    ElemPresentSwapChain(applicationPayload->SwapChain);

    SampleFrameMeasurement frameMeasurement = SampleEndFrameMeasurement();

    ElemGraphicsDeviceInfo graphicsDeviceInfo = ElemGetGraphicsDeviceInfo(applicationPayload->GraphicsDevice);
    SampleSetWindowTitle(applicationPayload->Window, "HelloTriangle", graphicsDeviceInfo, frameMeasurement.FrameTime, frameMeasurement.Fps);

    globalShaderParameters.AspectRatio = (float)renderSize.Width / renderSize.Height;
    globalShaderParameters.RotationY += 1.5f * frameMeasurement.FrameTime / 1000.0;
}

void InitApplication(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemWindow window = ElemCreateWindow(0, NULL);
    ElemWindowSize currentRenderSize = ElemGetWindowRenderSize(window);

    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = true, .PreferVulkan = applicationPayload->PreferVulkan });
    ElemGraphicsDevice graphicsDevice = ElemCreateGraphicsDevice(NULL);

    ElemCommandQueue commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, &(ElemCommandQueueOptions) { .DebugName = "TestCommandQueue" });
    ElemSwapChain swapChain = ElemCreateSwapChain2(commandQueue, window, UpdateSwapChain, &(ElemSwapChainOptions) { .UpdatePayload = payload });
    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(swapChain);

    ElemDataSpan shaderData = SampleReadFile(applicationPayload->ProgramPath, "Triangle.bin");
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
}

void FreeApplication(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);

    printf("Exit Sample\n");
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

    ElemRunApplication2(&(ElemRunApplicationParameters)
    {
        .ApplicationName = "Hello Triangle2",
        .InitHandler = InitApplication,
        .FreeHandler = FreeApplication,
        .Payload = &payload
    });
}

