#include "Elemental.h"
#include "ElementalTools.h"
#include "../Common/SampleUtils.h"

static ElemCommandQueue globalCommandQueue;
static ElemSwapChain globalSwapChain;
static ElemPipelineState globalGraphicsPipeline;

bool RunHandler(ElemApplicationStatus status)
{
    if (status == ElemApplicationStatus_Closing)
    {
        return false;
    }

    ElemWaitForSwapChainOnCpu(globalSwapChain);

    ElemCommandList commandList = ElemCreateCommandList(globalCommandQueue, &(ElemCommandListOptions) { .DebugName = "TestCommandList" }); 

    ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters) {
        .RenderTargets = 
        {
            .Items = (ElemRenderPassRenderTarget[]){ 
                {
                    ElemGetSwapChainBackBufferTexture(globalSwapChain),
                    .ClearColor = { 1.0f, 1.0f, 0.0f, 1.0f },
                    .LoadAction = ElemRenderPassLoadAction_Clear
                }
            },
            .Length = 1
        }
    });

    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);
    ElemExecuteCommandList(globalCommandQueue, commandList, NULL);

    ElemPresentSwapChain(globalSwapChain);

    return true;
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
    ElemWindow window = ElemCreateWindow(application, NULL);

    ElemGraphicsDevice graphicsDevice = ElemCreateGraphicsDevice(NULL);
    ElemGraphicsDeviceInfo graphicsDeviceInfo = ElemGetGraphicsDeviceInfo(graphicsDevice);

    char* shaderSource = ReadFileToString(argv[0], "Data/Triangle.hlsl");
    ElemShaderSourceData shaderSourceData = { .ShaderLanguage = ElemShaderLanguage_Hlsl, .Data = { .Items = (uint8_t*)shaderSource, .Length = strlen(shaderSource) } };
    //ElemShaderCompilationResult compilationResult = ElemCompileShaderLibrary(ElemToolsGraphicsApi_Metal, &shaderSourceData, &(ElemCompileShaderOptions) { .DebugMode = false });
    ElemShaderCompilationResult compilationResult = ElemCompileShaderLibrary((ElemToolsGraphicsApi)graphicsDeviceInfo.GraphicsApi, &shaderSourceData, &(ElemCompileShaderOptions) { .DebugMode = false });

    if (compilationResult.HasErrors)
    {
        printf("Errrrooooor!!!\n");
    }

    for (uint32_t i = 0; i < compilationResult.Messages.Length; i++)
    {
        printf("Compil msg (%d): %s\n", compilationResult.Messages.Items[i].Type, compilationResult.Messages.Items[i].Message);
    }

    char temp[255];
    sprintf(temp, "Hello Triangle! (GraphicsDevice: DeviceName=%s, GraphicsApi=%s, DeviceId=%llu, AvailableMemory=%llu)", 
                        graphicsDeviceInfo.DeviceName, 
                        GetGraphicsApiLabel(graphicsDeviceInfo.GraphicsApi),
                        graphicsDeviceInfo.DeviceId, 
                        graphicsDeviceInfo.AvailableMemory);
    ElemSetWindowTitle(window, temp);

    globalCommandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, &(ElemCommandQueueOptions) { .DebugName = "TestCommandQueue" });
    globalSwapChain = ElemCreateSwapChain(globalCommandQueue, window, &(ElemSwapChainOptions) { });

    //ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary((ElemDataContainer) { .Data = NULL, .Length = 1 });
    //globalGraphicsPipeline = ElemCreateGraphicsPipelineState(graphicsDevice, &(ElemGraphicsPipelineStateParameters) {
    //});

    ElemRunApplication(application, RunHandler);

    ElemFreeSwapChain(globalSwapChain);
    ElemFreeCommandQueue(globalCommandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);
    ElemFreeApplication(application);

    return 0;
}
