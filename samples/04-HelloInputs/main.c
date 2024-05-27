#include <math.h>
#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"

typedef struct
{
    float RotateLeft;
    float RotateRight;
    float RotateUp;
    float RotateDown;
    float RotateSideLeft;
    float RotateSideRight;

    float Touch;
    float TouchReleased;
    float TouchRotateSide;
    float TouchRotateSideReleased;
    float TouchRotateLeft;
    float TouchRotateRight;
    float TouchRotateUp;
    float TouchRotateDown;

    float ZoomIn;
    float ZoomOut;
    float ChangeColor;
    float HideCursor;
    float ShowCursor;

    float SwitchAcceleration;
    float ExitApp;
} InputActions;

typedef struct
{
    Vector4 RotationQuaternion;
    float Zoom;
    float AspectRatio;
    uint32_t TriangeColor;
} ShaderParameters;

typedef struct
{
    bool PreferVulkan;
    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    ElemSwapChain SwapChain;
    ElemPipelineState GraphicsPipeline;
    InputActions InputActions;
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
void UpdateInputValueNegate(ElemInputId inputId, const ElemInputEvent* inputEvent, float* inputValue)
{
    if (inputEvent->InputId == inputId)
    {
        *inputValue = !inputEvent->Value;
    }
}

void UpdateInputValue(ElemInputId inputId, const ElemInputEvent* inputEvent, float* inputValue)
{
    if (inputEvent->InputId == inputId)
    {
        *inputValue = inputEvent->Value;
    }
}

void UpdateInputs(InputActions* inputActions)
{
    ElemInputStream inputStream = ElemGetInputStream();

    inputActions->ExitApp = false;
    inputActions->TouchReleased = false;
    inputActions->TouchRotateSideReleased = false;
    inputActions->SwitchAcceleration = false;

    for (uint32_t i = 0; i < inputStream.Events.Length; i++)
    {
        ElemInputEvent* inputEvent = &inputStream.Events.Items[i];

        if (inputEvent->InputType != ElemInputType_Delta)
        {
            printf("Received an input event %d: Device=%lu, Value=%f (Elapsed: %f)\n", inputEvent->InputId, inputEvent->InputDevice, inputEvent->Value, inputEvent->ElapsedSeconds);
        }

        // TODO: Have a way to configure multiple keys for one event in one shot
        UpdateInputValue(ElemInputId_KeyA, inputEvent, &inputActions->RotateLeft);
        UpdateInputValue(ElemInputId_GamepadLeftStickXNegative, inputEvent, &inputActions->RotateLeft);

        UpdateInputValue(ElemInputId_KeyD, inputEvent, &inputActions->RotateRight);
        UpdateInputValue(ElemInputId_GamepadLeftStickXPositive, inputEvent, &inputActions->RotateRight);

        UpdateInputValue(ElemInputId_KeyW, inputEvent, &inputActions->RotateUp);
        UpdateInputValue(ElemInputId_GamepadLeftStickYPositive, inputEvent, &inputActions->RotateUp);

        UpdateInputValue(ElemInputId_KeyS, inputEvent, &inputActions->RotateDown);
        UpdateInputValue(ElemInputId_GamepadLeftStickYNegative, inputEvent, &inputActions->RotateDown);

        UpdateInputValue(ElemInputId_KeyQ, inputEvent, &inputActions->RotateSideLeft);
        UpdateInputValue(ElemInputID_GamepadLeftTrigger, inputEvent, &inputActions->RotateSideLeft);

        UpdateInputValue(ElemInputId_KeyE, inputEvent, &inputActions->RotateSideRight);
        UpdateInputValue(ElemInputID_GamepadRightTrigger, inputEvent, &inputActions->RotateSideRight);

        UpdateInputValue(ElemInputId_MouseLeftButton, inputEvent, &inputActions->Touch);
        UpdateInputValue(ElemInputId_MouseRightButton, inputEvent, &inputActions->TouchRotateSide);
        UpdateInputValue(ElemInputId_MouseAxisXNegative, inputEvent, &inputActions->TouchRotateLeft);
        UpdateInputValue(ElemInputId_MouseAxisXPositive, inputEvent, &inputActions->TouchRotateRight);
        UpdateInputValue(ElemInputId_MouseAxisYNegative, inputEvent, &inputActions->TouchRotateUp);
        UpdateInputValue(ElemInputId_MouseAxisYPositive, inputEvent, &inputActions->TouchRotateDown);
        UpdateInputValue(ElemInputId_TouchXNegative, inputEvent, &inputActions->TouchRotateLeft);
        UpdateInputValue(ElemInputId_TouchXPositive, inputEvent, &inputActions->TouchRotateRight);
        UpdateInputValue(ElemInputId_TouchYNegative, inputEvent, &inputActions->TouchRotateUp);
        UpdateInputValue(ElemInputId_TouchYPositive, inputEvent, &inputActions->TouchRotateDown);

        // TODO: For mouse wheel we should apply some scaling otherwise it is too slow
        UpdateInputValue(ElemInputId_KeyZ, inputEvent, &inputActions->ZoomIn);
        UpdateInputValue(ElemInputID_GamepadRightShoulder, inputEvent, &inputActions->ZoomIn);
        UpdateInputValue(ElemInputId_MouseWheelPositive, inputEvent, &inputActions->ZoomIn);

        UpdateInputValue(ElemInputId_KeyX, inputEvent, &inputActions->ZoomOut);
        UpdateInputValue(ElemInputID_GamepadLeftShoulder, inputEvent, &inputActions->ZoomOut);
        UpdateInputValue(ElemInputId_MouseWheelNegative, inputEvent, &inputActions->ZoomOut);

        UpdateInputValue(ElemInputId_KeySpacebar, inputEvent, &inputActions->ChangeColor);
        UpdateInputValue(ElemInputId_MouseMiddleButton, inputEvent, &inputActions->ChangeColor);
        UpdateInputValue(ElemInputID_GamepadButtonA, inputEvent, &inputActions->ChangeColor);
        UpdateInputValue(ElemInputId_GamepadLeftStickButton, inputEvent, &inputActions->ChangeColor);

        UpdateInputValue(ElemInputId_KeyF1, inputEvent, &inputActions->HideCursor);
        UpdateInputValue(ElemInputId_KeyF2, inputEvent, &inputActions->ShowCursor);
        UpdateInputValueNegate(ElemInputId_KeyF3, inputEvent, &inputActions->SwitchAcceleration);
        UpdateInputValueNegate(ElemInputId_KeyEscape, inputEvent, &inputActions->ExitApp);
        UpdateInputValueNegate(ElemInputId_MouseLeftButton, inputEvent, &inputActions->TouchReleased);
        UpdateInputValueNegate(ElemInputId_MouseRightButton, inputEvent, &inputActions->TouchRotateSideReleased);
        UpdateInputValueNegate(ElemInputId_Touch, inputEvent, &inputActions->TouchReleased);
    }
}

Vector3 rotationTouch = { };
float rotationTouchDecreaseSpeed = 0.001f;
float rotationTouchMaxSpeed = 0.3f;
bool useAcceleration = true;
Vector3 currentRotationSpeed = V3Zero;

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->ShaderParameters.AspectRatio = updateParameters->SwapChainInfo.AspectRatio;

    InputActions* inputActions = &applicationPayload->InputActions;
    UpdateInputs(inputActions);

    if (inputActions->ExitApp)
    {
        ElemExitApplication(0);
    }

    if (inputActions->SwitchAcceleration)
    {
        useAcceleration = !useAcceleration;
        printf("Acceleration: %d\n", useAcceleration);
    }

    //ElemWindowCursorPosition cursorPosition = ElemGetWindowCursorPosition(applicationPayload->Window);
    //printf("Cursor Position: %u, %u\n", cursorPosition.X, cursorPosition.Y);

    // TODO: Use an acceleration? (only for linear motions! Touch motion or mouse are already accelerated by the user :))
    Vector3 rotationDelta = V3Zero; 

    // TODO: On macbook pro we need to prioritize the mouse over the touchpad but keep touch into account

    // TODO: Can we add options to normalize the speed we need for different path?
    if (inputActions->Touch)
    {
        rotationDelta.X = (inputActions->TouchRotateUp - inputActions->TouchRotateDown) * 4.0f * updateParameters->DeltaTimeInSeconds;
        rotationDelta.Y = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * 4.0f * updateParameters->DeltaTimeInSeconds;

        // TODO: We should only zero the X and Y component!
        rotationTouch = V3Zero;
    }
    else
    {
        Vector3 direction = NormalizeV3((Vector3) 
        { 
            .X = (inputActions->RotateUp - inputActions->RotateDown),
            .Y = (inputActions->RotateLeft - inputActions->RotateRight),
            .Z = 0.0f
        });

        if (!useAcceleration)
        {
            rotationDelta = MulScalarV3(direction, 8.0f * updateParameters->DeltaTimeInSeconds);
        }
        else
        {
            // TODO: The friction here is what we do for the touch. So maybe we can take the same code but with different constants?
            Vector3 acceleration = AddV3(MulScalarV3(direction, 500.0f), MulScalarV3(InverseV3(currentRotationSpeed), 60));

            rotationDelta = AddV3(MulScalarV3(acceleration, 0.5f * pow2f(updateParameters->DeltaTimeInSeconds)), MulScalarV3(currentRotationSpeed, updateParameters->DeltaTimeInSeconds));
            currentRotationSpeed = AddV3(MulScalarV3(acceleration, updateParameters->DeltaTimeInSeconds), currentRotationSpeed);

            printf("Current Speed: %f %f %f\n", currentRotationSpeed.X, currentRotationSpeed.Y, currentRotationSpeed.Z);
        }
    }

    if (inputActions->TouchRotateSide)
    {
        rotationDelta.Z = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * 2.0f * updateParameters->DeltaTimeInSeconds;
    }
    else
    {
        rotationDelta.Z = (inputActions->RotateSideLeft - inputActions->RotateSideRight) * 5.0f * updateParameters->DeltaTimeInSeconds;
    }

    if (inputActions->TouchReleased || inputActions->TouchRotateSideReleased)
    {

        if (inputActions->TouchReleased)
        {
            rotationTouch.X = (inputActions->TouchRotateUp - inputActions->TouchRotateDown) * 4.0f * updateParameters->DeltaTimeInSeconds;
            rotationTouch.Y = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * 4.0f * updateParameters->DeltaTimeInSeconds;
        }

        if (inputActions->TouchRotateSideReleased)
        {
            rotationTouch.Z = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * 2.0f * updateParameters->DeltaTimeInSeconds;
        }

        if (MagnitudeV3(rotationTouch) > rotationTouchMaxSpeed)
        {
            rotationTouch = MulScalarV3(NormalizeV3(rotationTouch), rotationTouchMaxSpeed);
        }
    }

    if (MagnitudeSquaredV3(rotationTouch) > 0)
    {
        //printf("Touch Speed = %f, %f, %f (%f)\n", rotationTouch.X, rotationTouch.Y, rotationTouch.Z, MagnitudeV3(rotationTouch));
        rotationDelta = AddV3(rotationDelta, rotationTouch);

        Vector3 inverse = MulScalarV3(NormalizeV3(InverseV3(rotationTouch)), rotationTouchDecreaseSpeed);
        rotationTouch = AddV3(rotationTouch, inverse);

        if (MagnitudeV3(rotationTouch) < 0.001f)
        {
            rotationTouch = (Vector3){};
        }
    }
        
    //printf("Rotation Delta = %f, %f, %f, (%f)\n", rotationXDelta, rotationYDelta, rotationZDelta, inputActions->TouchRotateUp);

    if (MagnitudeSquaredV3(rotationDelta))
    {
        Vector4 rotationQuaternion = MulQuat(CreateQuaternion((Vector3){ 1, 0, 0 }, rotationDelta.X), MulQuat(CreateQuaternion((Vector3){ 0, 0, 1 }, rotationDelta.Z), CreateQuaternion((Vector3){ 0, 1, 0 }, rotationDelta.Y)));
        applicationPayload->ShaderParameters.RotationQuaternion = MulQuat(rotationQuaternion, applicationPayload->ShaderParameters.RotationQuaternion);
    }

    applicationPayload->ShaderParameters.Zoom += (inputActions->ZoomIn - inputActions->ZoomOut) * 5.0f * updateParameters->DeltaTimeInSeconds;
    applicationPayload->ShaderParameters.TriangeColor = inputActions->ChangeColor;

    if (inputActions->HideCursor)
    {
        ElemHideWindowCursor(applicationPayload->Window);
    }

    if (inputActions->ShowCursor)
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

