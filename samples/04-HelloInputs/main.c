#include "Elemental.h"
#include <math.h>
#include "../Common/SampleUtils.h"

typedef struct
{
    float X, Y, Z;
} Vector3;

typedef struct
{
    float X, Y, Z, W;
} Vector4;

typedef struct
{
    Vector4 RotationQuaternion;
    float Zoom;
    float AspectRatio;
    uint32_t TriangeColor;
} ShaderParameters;

// TODO: Rename this to InputActions
typedef struct
{
    float RotateLeft;
    float RotateRight;
    float RotateUp;
    float RotateDown;

    float TouchAction;
    float TouchRotateLeft;
    float TouchRotateRight;
    float TouchRotateUp;
    float TouchRotateDown;

    float ZoomIn;
    float ZoomOut;
    float ChangeColorAction;

    float EscapePressed;
    float EscapeReleased;
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

// TODO: Review all the maths
Vector4 CreateQuaternion(Vector3 v, float w)
{
	Vector4 result;
    
	result.X = v.X * sinf(w * 0.5f);
	result.Y = v.Y * sinf(w * 0.5f);
	result.Z = v.Z * sinf(w * 0.5f);
	result.W = cosf(w * 0.5f);

	return result;
}

Vector3 crossProduct(Vector4 v1, Vector4 v2) {
    Vector3 result;
    result.X = v1.Y * v2.Z - v1.Z * v2.Y;
    result.Y = v1.Z * v2.X - v1.X * v2.Z;
    result.Z = v1.X * v2.Y - v1.Y * v2.X;
    return result;
}
float dotProduct(Vector4 v1, Vector4 v2) {
    return v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z;
}
Vector4 qmul(Vector4 q1, Vector4 q2)
{
    float x = q2.X * q1.W + q1.X * q2.W + crossProduct(q1, q2).X;
    float y = q2.Y * q1.W + q1.Y * q2.W + crossProduct(q1, q2).Y;
    float z = q2.Z * q1.W + q1.Z * q2.W + crossProduct(q1, q2).Z;
    
    float w = q1.W * q2.W - dotProduct(q1, q2);

    return (Vector4) { x, y, z, w };
}

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

    applicationPayload->ShaderParameters.RotationQuaternion = (Vector4){ .X = 0, .Y = 0, .Z = 0, .W = 1 };
    
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

void UpdateInputValue(ElemInputId inputId, const ElemInputEvent* inputEvent, float* inputValue)
{
    if (inputEvent->InputId == inputId)
    {
        *inputValue = inputEvent->Value;
    }
}

void UpdateInputs(ApplicationPayload* applicationPayload)
{
    InputState* inputState = &applicationPayload->InputState;
    ElemInputStream inputStream = ElemGetInputStream(NULL);

    if (!applicationPayload->InputState.EscapePressed && applicationPayload->InputState.EscapeReleased)
    {
        applicationPayload->InputState.EscapeReleased = false;
    }

    for (uint32_t i = 0; i < inputStream.Events.Length; i++)
    {
        ElemInputEvent* inputEvent = &inputStream.Events.Items[i];

        if (inputEvent->InputType != ElemInputType_Delta)
        {
            printf("Received an input event %d: Device=%lu, Value=%f (Elapsed: %f)\n", inputEvent->InputId, inputEvent->InputDevice, inputEvent->Value, inputEvent->ElapsedSeconds);
        }

        // TODO: Have a way to configure multiple keys for one event in one shot
        UpdateInputValue(ElemInputId_KeyQ, inputEvent, &inputState->RotateLeft);
        UpdateInputValue(ElemInputId_GamepadLeftStickXNegative, inputEvent, &inputState->RotateLeft);

        UpdateInputValue(ElemInputId_KeyD, inputEvent, &inputState->RotateRight);
        UpdateInputValue(ElemInputId_GamepadLeftStickXPositive, inputEvent, &inputState->RotateRight);

        UpdateInputValue(ElemInputId_KeyZ, inputEvent, &inputState->RotateUp);
        UpdateInputValue(ElemInputId_GamepadLeftStickYNegative, inputEvent, &inputState->RotateUp);

        UpdateInputValue(ElemInputId_KeyS, inputEvent, &inputState->RotateDown);
        UpdateInputValue(ElemInputId_GamepadLeftStickYPositive, inputEvent, &inputState->RotateDown);

        UpdateInputValue(ElemInputId_MouseLeftButton, inputEvent, &inputState->TouchAction);
        UpdateInputValue(ElemInputId_MouseAxisXNegative, inputEvent, &inputState->TouchRotateLeft);
        UpdateInputValue(ElemInputId_MouseAxisXPositive, inputEvent, &inputState->TouchRotateRight);
        UpdateInputValue(ElemInputId_MouseAxisYNegative, inputEvent, &inputState->TouchRotateUp);
        UpdateInputValue(ElemInputId_MouseAxisYPositive, inputEvent, &inputState->TouchRotateDown);

        // TODO: For mouse wheel we should apply some scaling otherwise it is too slow
        UpdateInputValue(ElemInputId_KeyW, inputEvent, &inputState->ZoomIn);
        UpdateInputValue(ElemInputId_MouseWheelPositive, inputEvent, &inputState->ZoomIn);

        UpdateInputValue(ElemInputId_KeyX, inputEvent, &inputState->ZoomOut);
        UpdateInputValue(ElemInputId_MouseWheelNegative, inputEvent, &inputState->ZoomOut);

        UpdateInputValue(ElemInputId_KeySpace, inputEvent, &inputState->ChangeColorAction);
        UpdateInputValue(ElemInputId_MouseMiddleButton, inputEvent, &inputState->ChangeColorAction);
        UpdateInputValue(ElemInputID_GamepadButton1, inputEvent, &inputState->ChangeColorAction);

        if (inputEvent->InputId == ElemInputId_KeyEscape)
        {
            if (applicationPayload->InputState.EscapePressed && inputEvent->Value == 0.0f)
            {
                applicationPayload->InputState.EscapeReleased = true;
            }

            applicationPayload->InputState.EscapePressed = inputEvent->Value;
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

    // TODO: Use an acceleration?
    float rotationXDelta = 0.0f;
    float rotationYDelta = 0.0f;

    // TODO: Can we add options to normalize the speed we need for different path?
    if (applicationPayload->InputState.TouchAction)
    {
        rotationXDelta = (applicationPayload->InputState.TouchRotateUp - applicationPayload->InputState.TouchRotateDown) * 2.5f * updateParameters->DeltaTimeInSeconds;
        rotationYDelta = (applicationPayload->InputState.TouchRotateLeft - applicationPayload->InputState.TouchRotateRight) * 2.5f * updateParameters->DeltaTimeInSeconds;
    }
    else
    {
        rotationXDelta = (applicationPayload->InputState.RotateUp - applicationPayload->InputState.RotateDown) * 5.0f * updateParameters->DeltaTimeInSeconds;
        rotationYDelta = (applicationPayload->InputState.RotateLeft - applicationPayload->InputState.RotateRight) * 5.0f * updateParameters->DeltaTimeInSeconds;
    }

    if (rotationXDelta || rotationYDelta)
    {
        Vector4 rotationQuaternion = qmul(CreateQuaternion((Vector3){ 1, 0, 0 }, rotationXDelta), CreateQuaternion((Vector3){ 0, 1, 0 }, rotationYDelta));
        applicationPayload->ShaderParameters.RotationQuaternion = qmul(rotationQuaternion, applicationPayload->ShaderParameters.RotationQuaternion);
    }

    // TODO: Z rotation

    applicationPayload->ShaderParameters.Zoom += (applicationPayload->InputState.ZoomIn - applicationPayload->InputState.ZoomOut) * 5.0f * updateParameters->DeltaTimeInSeconds;
    applicationPayload->ShaderParameters.TriangeColor = applicationPayload->InputState.ChangeColorAction;

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

