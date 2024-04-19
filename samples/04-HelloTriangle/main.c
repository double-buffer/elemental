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

bool isLatencyUpdated = false;

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    
    if (updateParameters->NextPresentTimeStampInSeconds > 4 && !isLatencyUpdated)
    {
        ElemSetSwapChainTiming(applicationPayload->SwapChain, 1, 30);
        isLatencyUpdated = true;
    }

    //printf("Next Present Timestamp in seconds: %f\n", updateParameters->NextPresentTimeStampInSeconds);
    //printf("DeltaTime in seconds: %f\n", updateParameters->DeltaTimeInSeconds);
    //Sleep(7);

    ElemCommandList commandList = ElemGetCommandList(applicationPayload->CommandQueue, &(ElemCommandListOptions) { .DebugName = "TestCommandList" }); 

    ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters) {
        .RenderTargets = 
        {
            .Items = (ElemRenderPassRenderTarget[]) { 
            {
                .RenderTarget = updateParameters->BackBufferTexture, 
                .ClearColor = { 0.0f, 0.01f, 0.02f, 1.0f },
                .LoadAction = ElemRenderPassLoadAction_Clear
            }},
            .Length = 1
        },
        /*.Viewports = 
        {
            .Items = (ElemViewport[]) {
            {
                .X = 200, 
                .Y = 200,
                .Width = 800, 
                .Height = 600, 
                .MinDepth = 0, 
                .MaxDepth = 1
            }},
            .Length = 1
        }*/
    });

    ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline);
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });

    /*
    ElemSetViewport(commandList, &(ElemViewport)
    {
        .X = 200, 
        .Y = 200,
        .Width = 800, 
        .Height = 600, 
        .MinDepth = 0, 
        .MaxDepth = 1
    });*/

    ElemDispatchMesh(commandList, 1, 1, 1);

    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);
    ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, NULL);

    ElemPresentSwapChain(applicationPayload->SwapChain);

    // NOTE: This is the update part of the loop. If render fast this will create a gap and add latency because we need to wait for the next available
    // Backbuffer
    SampleFrameMeasurement frameMeasurement = SampleEndFrameMeasurement();

    ElemGraphicsDeviceInfo graphicsDeviceInfo = ElemGetGraphicsDeviceInfo(applicationPayload->GraphicsDevice);
    SampleSetWindowTitle(applicationPayload->Window, "HelloTriangle", graphicsDeviceInfo, frameMeasurement.FrameTimeInSeconds, frameMeasurement.Fps);

    applicationPayload->ShaderParameters.AspectRatio = (float)updateParameters->SwapChainInfo.Width / updateParameters->SwapChainInfo.Height;
    applicationPayload->ShaderParameters.RotationY += 1.5f * (float)updateParameters->DeltaTimeInSeconds;
    
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

