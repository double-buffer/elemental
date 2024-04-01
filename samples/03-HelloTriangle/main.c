#include "Elemental.h"
#include "ElementalTools.h"
#include "../Common/SampleUtils.h"

typedef struct
{
    float AspectRatio;
    float RotationY;
} ShaderParameters;

static ElemWindow globalWindow;
static ElemWindowSize globalCurrentRenderSize;
static ElemGraphicsDevice globalGraphicsDevice;
static ElemCommandQueue globalCommandQueue;
static ElemSwapChain globalSwapChain;
static ElemPipelineState globalGraphicsPipeline;

static ShaderParameters globalShaderParameters;

bool RunHandler(ElemApplicationStatus status);

ElemShaderLibrary CompileShader(ElemGraphicsDevice graphicsDevice, const char* programPath, const char* shaderPath)
{
    ElemGraphicsDeviceInfo graphicsDeviceInfo = ElemGetGraphicsDeviceInfo(graphicsDevice);

    char* shaderSource = SampleReadFileToString(programPath, shaderPath);
    ElemShaderSourceData shaderSourceData = { .ShaderLanguage = ElemShaderLanguage_Hlsl, .Data = { .Items = (uint8_t*)shaderSource, .Length = strlen(shaderSource) } };
    //ElemShaderCompilationResult compilationResult = ElemCompileShaderLibrary(ElemToolsGraphicsApi_Metal, &shaderSourceData, &(ElemCompileShaderOptions) { .DebugMode = false });
    ElemShaderCompilationResult compilationResult = ElemCompileShaderLibrary((ElemToolsGraphicsApi)graphicsDeviceInfo.GraphicsApi, &shaderSourceData, &(ElemCompileShaderOptions) { .DebugMode = false });

    for (uint32_t i = 0; i < compilationResult.Messages.Length; i++)
    {
        printf("Compil msg (%d): %s\n", compilationResult.Messages.Items[i].Type, compilationResult.Messages.Items[i].Message);
    }

    if (compilationResult.HasErrors)
    {
        exit(1);
    }

    return ElemCreateShaderLibrary(graphicsDevice, (ElemDataSpan) { .Items = compilationResult.Data.Items, .Length = compilationResult.Data.Length });
}

int main(int argc, const char* argv[]) 
{
    bool useVulkan = false;

    if (argc > 1 && strcmp(argv[1], "--vulkan") == 0)
    {
        useVulkan = true;
    }

    ElemConfigureLogHandler(ElemConsoleLogHandler);
    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = true, .PreferVulkan = useVulkan });
    
    ElemApplication application = ElemCreateApplication("Hello Triangle");
    globalWindow = ElemCreateWindow(application, NULL);
    globalCurrentRenderSize = ElemGetWindowRenderSize(globalWindow);

    globalGraphicsDevice = ElemCreateGraphicsDevice(NULL);

    globalCommandQueue = ElemCreateCommandQueue(globalGraphicsDevice, ElemCommandQueueType_Graphics, &(ElemCommandQueueOptions) { .DebugName = "TestCommandQueue" });
    globalSwapChain = ElemCreateSwapChain(globalCommandQueue, globalWindow, &(ElemSwapChainOptions) { });
    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(globalSwapChain);

    ElemShaderLibrary shaderLibrary = CompileShader(globalGraphicsDevice, argv[0], "Data/Triangle.hlsl");

    globalGraphicsPipeline = ElemCompileGraphicsPipelineState(globalGraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "Test PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .TextureFormats = { .Items = (ElemTextureFormat[]) { swapChainInfo.Format }, .Length = 1 }
    });

    ElemFreeShaderLibrary(shaderLibrary);

    ElemRunApplication(application, RunHandler);

    ElemFreePipelineState(globalGraphicsPipeline);
    ElemFreeSwapChain(globalSwapChain);
    ElemFreeCommandQueue(globalCommandQueue);
    ElemFreeGraphicsDevice(globalGraphicsDevice);
    ElemFreeApplication(application);

    return 0;
}

bool RunHandler(ElemApplicationStatus status)
{
    if (status == ElemApplicationStatus_Closing)
    {
        return false;
    }

    SampleStartFrameMeasurement();
    ElemWaitForSwapChainOnCpu(globalSwapChain);

    ElemWindowSize renderSize = ElemGetWindowRenderSize(globalWindow);

    if (renderSize.Width != globalCurrentRenderSize.Width || renderSize.Height != globalCurrentRenderSize.Height)
    {
        ElemResizeSwapChain(globalSwapChain, renderSize.Width, renderSize.Height);
        globalCurrentRenderSize = renderSize;
    }

    ElemCommandList commandList = ElemGetCommandList(globalCommandQueue, &(ElemCommandListOptions) { .DebugName = "TestCommandList" }); 

    ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters) {
        .RenderTargets = 
        {
            .Items = (ElemRenderPassRenderTarget[]){ 
                {
                    ElemGetSwapChainBackBufferTexture(globalSwapChain),
                    .ClearColor = { 0.0f, 0.01f, 0.02f, 1.0f },
                    .LoadAction = ElemRenderPassLoadAction_Clear
                }
            },
            .Length = 1
        }
    });

    ElemBindPipelineState(commandList, globalGraphicsPipeline);
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&globalShaderParameters, .Length = sizeof(ShaderParameters) });
    ElemDispatchMesh(commandList, 1, 1, 1);

    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);
    ElemExecuteCommandList(globalCommandQueue, commandList, NULL);

    ElemPresentSwapChain(globalSwapChain);
    SampleFrameMeasurement frameMeasurement = SampleEndFrameMeasurement();

    ElemGraphicsDeviceInfo graphicsDeviceInfo = ElemGetGraphicsDeviceInfo(globalGraphicsDevice);
    SampleSetWindowTitle(globalWindow, "HelloTriangle", graphicsDeviceInfo, frameMeasurement.FrameTime, frameMeasurement.Fps);

    globalShaderParameters.AspectRatio = (float)renderSize.Width / renderSize.Height;
    globalShaderParameters.RotationY += 1.5f * frameMeasurement.FrameTime / 1000.0;

    return true;
}
