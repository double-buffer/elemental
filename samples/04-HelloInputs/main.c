#include "Elemental.h"
#include "../Common/SampleUtils.h"

typedef struct
{
    float AspectRatio;
    float RotationX;
    float RotationY;
} ShaderParameters;

typedef struct
{
    bool KeyUpPressed;
    bool KeyDownPressed; 
    bool KeyLeftPressed;
    bool KeyRightPressed;
    bool EscapePressed;
    bool EscapeReleased;
} InputState;

typedef struct
{
    bool PreferVulkan;
    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    ElemSwapChain SwapChain;
    ElemPipelineState GraphicsPipeline;
    InputState InputState;
    ShaderParameters ShaderParameters;
} ApplicationPayload;

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    applicationPayload->Window = ElemCreateWindow(NULL);

    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = true, .PreferVulkan = applicationPayload->PreferVulkan });
    applicationPayload->GraphicsDevice = ElemCreateGraphicsDevice(NULL);

    applicationPayload->CommandQueue= ElemCreateCommandQueue(applicationPayload->GraphicsDevice, ElemCommandQueueType_Graphics, NULL);
    applicationPayload->SwapChain= ElemCreateSwapChain(applicationPayload->CommandQueue, applicationPayload->Window, UpdateSwapChain, &(ElemSwapChainOptions) { .UpdatePayload = payload });
    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(applicationPayload->SwapChain);

    ElemDataSpan shaderData = SampleReadFile(!applicationPayload->PreferVulkan ? "Triangle.shader": "Triangle_vulkan.shader");
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, shaderData);

    applicationPayload->GraphicsPipeline = ElemCompileGraphicsPipelineState(applicationPayload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "Test PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .TextureFormats = { .Items = (ElemTextureFormat[]) { swapChainInfo.Format }, .Length = 1 }
    });
    
    ElemFreeShaderLibrary(shaderLibrary);
    SampleStartFrameMeasurement();
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);
}

void UpdateInputs(ApplicationPayload* applicationPayload)
{
    ElemInputStream inputStream = ElemGetInputStream(NULL);

    if (!applicationPayload->InputState.EscapePressed && applicationPayload->InputState.EscapeReleased)
    {
        applicationPayload->InputState.EscapeReleased = false;
    }

    //printf("Input Stream Timestamp: %f\n", inputStream.TimestampInSeconds);

    for (uint32_t i = 0; i < inputStream.Events.Length; i++)
    {
        printf("Received an input event: Value=%f\n", inputStream.Events.Items[i].Value);

        if (inputStream.Events.Items[i].InputId == ElemInputId_KeyD)
        {
            applicationPayload->InputState.KeyRightPressed = inputStream.Events.Items[i].Value;
        }

        if (inputStream.Events.Items[i].InputId == ElemInputId_KeyQ)
        {
            applicationPayload->InputState.KeyLeftPressed = inputStream.Events.Items[i].Value;
        }

        if (inputStream.Events.Items[i].InputId == ElemInputId_KeyS)
        {
            applicationPayload->InputState.KeyDownPressed = inputStream.Events.Items[i].Value;
        }

        if (inputStream.Events.Items[i].InputId == ElemInputId_KeyZ)
        {
            applicationPayload->InputState.KeyUpPressed = inputStream.Events.Items[i].Value;
        }

        if (inputStream.Events.Items[i].InputId == ElemInputId_Escape)
        {
            if (applicationPayload->InputState.EscapePressed && inputStream.Events.Items[i].Value == 0.0f)
            {
                applicationPayload->InputState.EscapeReleased = true;
            }

            applicationPayload->InputState.EscapePressed = inputStream.Events.Items[i].Value;
        }
    }
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    UpdateInputs(applicationPayload);

    if (applicationPayload->InputState.EscapeReleased)
    {
        ElemExitApplication(0);
    }

    applicationPayload->ShaderParameters.RotationX += (applicationPayload->InputState.KeyDownPressed - applicationPayload->InputState.KeyUpPressed) * 2.5f * updateParameters->DeltaTimeInSeconds;
    applicationPayload->ShaderParameters.RotationY += (applicationPayload->InputState.KeyRightPressed - applicationPayload->InputState.KeyLeftPressed) * 2.5f * updateParameters->DeltaTimeInSeconds;

    ElemCommandList commandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL); 

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

    SampleSetWindowTitle(applicationPayload->Window, "HelloInputs", applicationPayload->GraphicsDevice, frameMeasurement.FrameTimeInSeconds, frameMeasurement.Fps);

    applicationPayload->ShaderParameters.AspectRatio = updateParameters->SwapChainInfo.AspectRatio;
    
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
        .ApplicationName = "Hello Inputs",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });
}

