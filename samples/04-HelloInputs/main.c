#include <math.h>
#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"

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
    float RotateSideLeft;
    float RotateSideRight;

    float TouchAction;
    float TouchActionReleased;
    float TouchRotateSideAction;
    float TouchRotateSideActionReleased;
    float TouchRotateLeft;
    float TouchRotateRight;
    float TouchRotateUp;
    float TouchRotateDown;

    float ZoomIn;
    float ZoomOut;
    float ChangeColorAction;
    float HideCursorAction;
    float ShowCursorAction;
    float SwitchAcceleration;
    float SwitchAccelerationReleased;

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

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    //applicationPayload->Window = ElemCreateWindow(&(ElemWindowOptions) { .IsCursorHidden = true, .WindowState = ElemWindowState_FullScreen });
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
    ElemInputStream inputStream = ElemGetInputStream();

    if (!applicationPayload->InputState.EscapePressed && applicationPayload->InputState.EscapeReleased)
    {
        applicationPayload->InputState.EscapeReleased = false;
    }

    if (!applicationPayload->InputState.TouchAction && applicationPayload->InputState.TouchActionReleased)
    {
        applicationPayload->InputState.TouchActionReleased = false;
    }

    if (!applicationPayload->InputState.TouchRotateSideAction && applicationPayload->InputState.TouchRotateSideActionReleased)
    {
        applicationPayload->InputState.TouchRotateSideActionReleased = false;
    }
    if (!applicationPayload->InputState.SwitchAcceleration && applicationPayload->InputState.SwitchAccelerationReleased)
    {
        applicationPayload->InputState.SwitchAccelerationReleased = false;
    }

    for (uint32_t i = 0; i < inputStream.Events.Length; i++)
    {
        ElemInputEvent* inputEvent = &inputStream.Events.Items[i];

        if (inputEvent->InputType != ElemInputType_Delta)
        {
            printf("Received an input event %d: Device=%lu, Value=%f (Elapsed: %f)\n", inputEvent->InputId, inputEvent->InputDevice, inputEvent->Value, inputEvent->ElapsedSeconds);
        }

        // TODO: Have a way to configure multiple keys for one event in one shot
        UpdateInputValue(ElemInputId_KeyA, inputEvent, &inputState->RotateLeft);
        UpdateInputValue(ElemInputId_GamepadLeftStickXNegative, inputEvent, &inputState->RotateLeft);

        UpdateInputValue(ElemInputId_KeyD, inputEvent, &inputState->RotateRight);
        UpdateInputValue(ElemInputId_GamepadLeftStickXPositive, inputEvent, &inputState->RotateRight);

        UpdateInputValue(ElemInputId_KeyW, inputEvent, &inputState->RotateUp);
        UpdateInputValue(ElemInputId_GamepadLeftStickYPositive, inputEvent, &inputState->RotateUp);

        UpdateInputValue(ElemInputId_KeyS, inputEvent, &inputState->RotateDown);
        UpdateInputValue(ElemInputId_GamepadLeftStickYNegative, inputEvent, &inputState->RotateDown);

        UpdateInputValue(ElemInputId_KeyQ, inputEvent, &inputState->RotateSideLeft);
        UpdateInputValue(ElemInputID_GamepadLeftTrigger, inputEvent, &inputState->RotateSideLeft);

        UpdateInputValue(ElemInputId_KeyE, inputEvent, &inputState->RotateSideRight);
        UpdateInputValue(ElemInputID_GamepadRightTrigger, inputEvent, &inputState->RotateSideRight);

        //UpdateInputValue(ElemInputId_MouseLeftButton, inputEvent, &inputState->TouchAction);
        //UpdateInputValue(ElemInputId_MouseRightButton, inputEvent, &inputState->TouchRotateSideAction);
        UpdateInputValue(ElemInputId_MouseAxisXNegative, inputEvent, &inputState->TouchRotateLeft);
        UpdateInputValue(ElemInputId_MouseAxisXPositive, inputEvent, &inputState->TouchRotateRight);
        UpdateInputValue(ElemInputId_MouseAxisYNegative, inputEvent, &inputState->TouchRotateUp);
        UpdateInputValue(ElemInputId_MouseAxisYPositive, inputEvent, &inputState->TouchRotateDown);
        UpdateInputValue(ElemInputId_TouchXNegative, inputEvent, &inputState->TouchRotateLeft);
        UpdateInputValue(ElemInputId_TouchXPositive, inputEvent, &inputState->TouchRotateRight);
        UpdateInputValue(ElemInputId_TouchYNegative, inputEvent, &inputState->TouchRotateUp);
        UpdateInputValue(ElemInputId_TouchYPositive, inputEvent, &inputState->TouchRotateDown);

        // TODO: For mouse wheel we should apply some scaling otherwise it is too slow
        UpdateInputValue(ElemInputId_KeyZ, inputEvent, &inputState->ZoomIn);
        UpdateInputValue(ElemInputID_GamepadRightShoulder, inputEvent, &inputState->ZoomIn);
        UpdateInputValue(ElemInputId_MouseWheelPositive, inputEvent, &inputState->ZoomIn);

        UpdateInputValue(ElemInputId_KeyX, inputEvent, &inputState->ZoomOut);
        UpdateInputValue(ElemInputID_GamepadLeftShoulder, inputEvent, &inputState->ZoomOut);
        UpdateInputValue(ElemInputId_MouseWheelNegative, inputEvent, &inputState->ZoomOut);

        UpdateInputValue(ElemInputId_KeySpacebar, inputEvent, &inputState->ChangeColorAction);
        UpdateInputValue(ElemInputId_MouseMiddleButton, inputEvent, &inputState->ChangeColorAction);
        UpdateInputValue(ElemInputID_GamepadButtonA, inputEvent, &inputState->ChangeColorAction);
        UpdateInputValue(ElemInputId_GamepadLeftStickButton, inputEvent, &inputState->ChangeColorAction);

        UpdateInputValue(ElemInputId_KeyF1, inputEvent, &inputState->HideCursorAction);
        UpdateInputValue(ElemInputId_KeyF2, inputEvent, &inputState->ShowCursorAction);

        if (inputEvent->InputId == ElemInputId_KeyEscape)
        {
            if (applicationPayload->InputState.EscapePressed && inputEvent->Value == 0.0f)
            {
                applicationPayload->InputState.EscapeReleased = true;
            }

            applicationPayload->InputState.EscapePressed = inputEvent->Value;
        }

        if (inputEvent->InputId == ElemInputId_MouseLeftButton)
        {
            if (applicationPayload->InputState.TouchAction && inputEvent->Value == 0.0f)
            {
                applicationPayload->InputState.TouchActionReleased = true;
            }

            applicationPayload->InputState.TouchAction = inputEvent->Value;
        }

        if (inputEvent->InputId == ElemInputId_Touch)
        {
            if (applicationPayload->InputState.TouchAction && inputEvent->Value == 0.0f)
            {
                applicationPayload->InputState.TouchActionReleased = true;
            }

            applicationPayload->InputState.TouchAction = inputEvent->Value;
        }

        if (inputEvent->InputId == ElemInputId_MouseRightButton)
        {
            if (applicationPayload->InputState.TouchRotateSideAction && inputEvent->Value == 0.0f)
            {
                applicationPayload->InputState.TouchRotateSideActionReleased = true;
            }

            applicationPayload->InputState.TouchRotateSideAction = inputEvent->Value;
        }

        if (inputEvent->InputId == ElemInputId_KeyF3)
        {
            if (applicationPayload->InputState.SwitchAcceleration && inputEvent->Value == 0.0f)
            {
                applicationPayload->InputState.SwitchAccelerationReleased = true;
            }

            applicationPayload->InputState.SwitchAcceleration = inputEvent->Value;
        }
    }
}

Vector3 rotationTouch = { };
float rotationTouchDecreaseSpeed = 0.001f;
float rotationTouchMaxSpeed = 0.3f;
bool useAcceleration = true;
float currentSpeed = 0.0f;

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->ShaderParameters.AspectRatio = updateParameters->SwapChainInfo.AspectRatio;

    UpdateInputs(applicationPayload);

    if (applicationPayload->InputState.EscapeReleased)
    {
        ElemExitApplication(0);
    }

    if (applicationPayload->InputState.SwitchAccelerationReleased)
    {
        useAcceleration = !useAcceleration;
        printf("Acceleration: %d\n", useAcceleration);
    }

    //ElemWindowCursorPosition cursorPosition = ElemGetWindowCursorPosition(applicationPayload->Window);
    //printf("Cursor Position: %u, %u\n", cursorPosition.X, cursorPosition.Y);

    // TODO: Use an acceleration? (only for linear motions! Touch motion or mouse are already accelerated by the user :))
    float rotationXDelta = 0.0f;
    float rotationYDelta = 0.0f;
    float rotationZDelta = 0.0f;

    // TODO: On macbook pro we need to prioritize the mouse over the touchpad but keep touch into account

    // TODO: Can we add options to normalize the speed we need for different path?
    if (applicationPayload->InputState.TouchAction)
    {
        rotationXDelta = (applicationPayload->InputState.TouchRotateUp - applicationPayload->InputState.TouchRotateDown) * 4.0f * updateParameters->DeltaTimeInSeconds;
        rotationYDelta = (applicationPayload->InputState.TouchRotateLeft - applicationPayload->InputState.TouchRotateRight) * 4.0f * updateParameters->DeltaTimeInSeconds;
    }
    else
    {
        float acceleration = 30.0f;
        float speed = 8.0f;

        float xDirection = (applicationPayload->InputState.RotateUp - applicationPayload->InputState.RotateDown);
        float yDirection = (applicationPayload->InputState.RotateLeft - applicationPayload->InputState.RotateRight);

        if (!useAcceleration)
        {
            rotationXDelta = xDirection * speed * updateParameters->DeltaTimeInSeconds;
            rotationYDelta = yDirection * speed * updateParameters->DeltaTimeInSeconds;
        }
        else
        {
            rotationXDelta = 0.5f * acceleration * xDirection * pow2f(updateParameters->DeltaTimeInSeconds) + xDirection * currentSpeed * updateParameters->DeltaTimeInSeconds;
            rotationYDelta = 0.5f * acceleration * yDirection * pow2f(updateParameters->DeltaTimeInSeconds) + yDirection * currentSpeed * updateParameters->DeltaTimeInSeconds;

            if (xDirection || yDirection)
            {
                currentSpeed += acceleration * updateParameters->DeltaTimeInSeconds;
                currentSpeed = fminf(currentSpeed, speed);
            }
            else
            {
                currentSpeed = 0.0f;
            }
        }
    }

    if (applicationPayload->InputState.TouchRotateSideAction)
    {
        rotationZDelta = (applicationPayload->InputState.TouchRotateLeft - applicationPayload->InputState.TouchRotateRight) * 2.0f * updateParameters->DeltaTimeInSeconds;
    }
    else
    {
        rotationZDelta = (applicationPayload->InputState.RotateSideLeft - applicationPayload->InputState.RotateSideRight) * 5.0f * updateParameters->DeltaTimeInSeconds;
    }

    if (applicationPayload->InputState.TouchActionReleased || applicationPayload->InputState.TouchRotateSideActionReleased)
    {

        if (applicationPayload->InputState.TouchActionReleased)
        {
            rotationTouch.X = (applicationPayload->InputState.TouchRotateUp - applicationPayload->InputState.TouchRotateDown) * 4.0f * updateParameters->DeltaTimeInSeconds;
            rotationTouch.Y = (applicationPayload->InputState.TouchRotateLeft - applicationPayload->InputState.TouchRotateRight) * 4.0f * updateParameters->DeltaTimeInSeconds;
        }

        if (applicationPayload->InputState.TouchRotateSideActionReleased)
        {
            rotationTouch.Z = (applicationPayload->InputState.TouchRotateLeft - applicationPayload->InputState.TouchRotateRight) * 2.0f * updateParameters->DeltaTimeInSeconds;
        }

        if (MagnitudeV3(rotationTouch) > rotationTouchMaxSpeed)
        {
            rotationTouch = MulScalarV3(NormalizeV3(rotationTouch), rotationTouchMaxSpeed);
        }
    }

    if (MagnitudeSquaredV3(rotationTouch) > 0)
    {
        //printf("Touch Speed = %f, %f, %f (%f)\n", rotationTouch.X, rotationTouch.Y, rotationTouch.Z, MagnitudeV3(rotationTouch));
        rotationXDelta += rotationTouch.X;
        rotationYDelta += rotationTouch.Y;
        rotationZDelta += rotationTouch.Z;

        Vector3 inverse = MulScalarV3(NormalizeV3(InverseV3(rotationTouch)), rotationTouchDecreaseSpeed);
        rotationTouch = AddV3(rotationTouch, inverse);

        if (MagnitudeV3(rotationTouch) < 0.001f)
        {
            rotationTouch = (Vector3){};
        }
    }
        
    //printf("Rotation Delta = %f, %f, %f, (%f)\n", rotationXDelta, rotationYDelta, rotationZDelta, applicationPayload->InputState.TouchRotateUp);

    if (rotationXDelta || rotationYDelta || rotationZDelta)
    {
        Vector4 rotationQuaternion = MulQuat(CreateQuaternion((Vector3){ 1, 0, 0 }, rotationXDelta), MulQuat(CreateQuaternion((Vector3){ 0, 0, 1 }, rotationZDelta), CreateQuaternion((Vector3){ 0, 1, 0 }, rotationYDelta)));
        applicationPayload->ShaderParameters.RotationQuaternion = MulQuat(rotationQuaternion, applicationPayload->ShaderParameters.RotationQuaternion);
    }

    applicationPayload->ShaderParameters.Zoom += (applicationPayload->InputState.ZoomIn - applicationPayload->InputState.ZoomOut) * 5.0f * updateParameters->DeltaTimeInSeconds;
    applicationPayload->ShaderParameters.TriangeColor = applicationPayload->InputState.ChangeColorAction;

    if (applicationPayload->InputState.HideCursorAction)
    {
        ElemHideWindowCursor(applicationPayload->Window);
    }

    if (applicationPayload->InputState.ShowCursorAction)
    {
        ElemShowWindowCursor(applicationPayload->Window);
    }

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

    if (frameMeasurement.NewData)
    {
        SampleSetWindowTitle(applicationPayload->Window, "HelloInputs", applicationPayload->GraphicsDevice, frameMeasurement.FrameTimeInSeconds, frameMeasurement.Fps);
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
        .ApplicationName = "Hello Inputs",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });
}

