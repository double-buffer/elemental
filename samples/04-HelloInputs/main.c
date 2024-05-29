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
    float TouchPositionX;
    float TouchPositionY;

    float Touch2;
    float Touch2Released;
    float Touch2RotateLeft;
    float Touch2RotateRight;
    float Touch2RotateUp;
    float Touch2RotateDown;
    float Touch2PositionX;
    float Touch2PositionY;

    float ZoomIn;
    float ZoomOut;
    float ChangeColor;
    float HideCursor;
    float ShowCursor;

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

void UpdateInputValueNegate(ElemInputId inputId, uint32_t index, const ElemInputEvent* inputEvent, float* inputValue)
{
    if (inputEvent->InputId == inputId && inputEvent->InputDeviceTypeIndex == index)
    {
        *inputValue = !inputEvent->Value;
    }
}

void UpdateInputValue(ElemInputId inputId, uint32_t index, const ElemInputEvent* inputEvent, float* inputValue)
{
    if (inputEvent->InputId == inputId && inputEvent->InputDeviceTypeIndex == index)
    {
        *inputValue = inputEvent->Value;
    }
}

void UpdateInputs(InputActions* inputActions)
{
    ElemInputStream inputStream = ElemGetInputStream();

    inputActions->ExitApp = false;
    inputActions->TouchReleased = false;
    inputActions->Touch2Released = false;
    inputActions->TouchRotateSideReleased = false;

    for (uint32_t i = 0; i < inputStream.Events.Length; i++)
    {
        ElemInputEvent* inputEvent = &inputStream.Events.Items[i];

        if (inputEvent->InputType != ElemInputType_Delta && inputEvent->InputType != ElemInputType_Absolute)
        {
            printf("Received an input event %d: Device=%lu, Value=%f (Elapsed: %f)\n", inputEvent->InputId, inputEvent->InputDevice, inputEvent->Value, inputEvent->ElapsedSeconds);
        }

        // TODO: Have a way to configure multiple keys for one event in one shot
        UpdateInputValue(ElemInputId_KeyA, 0, inputEvent, &inputActions->RotateLeft);
        UpdateInputValue(ElemInputId_GamepadLeftStickXNegative, 0, inputEvent, &inputActions->RotateLeft);

        UpdateInputValue(ElemInputId_KeyD, 0, inputEvent, &inputActions->RotateRight);
        UpdateInputValue(ElemInputId_GamepadLeftStickXPositive, 0, inputEvent, &inputActions->RotateRight);

        UpdateInputValue(ElemInputId_KeyW, 0, inputEvent, &inputActions->RotateUp);
        UpdateInputValue(ElemInputId_GamepadLeftStickYPositive, 0, inputEvent, &inputActions->RotateUp);

        UpdateInputValue(ElemInputId_KeyS, 0, inputEvent, &inputActions->RotateDown);
        UpdateInputValue(ElemInputId_GamepadLeftStickYNegative, 0, inputEvent, &inputActions->RotateDown);

        UpdateInputValue(ElemInputId_KeyQ, 0, inputEvent, &inputActions->RotateSideLeft);
        UpdateInputValue(ElemInputID_GamepadLeftTrigger, 0, inputEvent, &inputActions->RotateSideLeft);

        UpdateInputValue(ElemInputId_KeyE, 0, inputEvent, &inputActions->RotateSideRight);
        UpdateInputValue(ElemInputID_GamepadRightTrigger, 0, inputEvent, &inputActions->RotateSideRight);

        UpdateInputValue(ElemInputId_MouseLeftButton, 0, inputEvent, &inputActions->Touch);
        UpdateInputValue(ElemInputId_Touch, 0, inputEvent, &inputActions->Touch);
        UpdateInputValue(ElemInputId_MouseRightButton, 0, inputEvent, &inputActions->TouchRotateSide);
        UpdateInputValue(ElemInputId_MouseAxisXNegative, 0, inputEvent, &inputActions->TouchRotateLeft);
        UpdateInputValue(ElemInputId_MouseAxisXPositive, 0, inputEvent, &inputActions->TouchRotateRight);
        UpdateInputValue(ElemInputId_MouseAxisYNegative, 0, inputEvent, &inputActions->TouchRotateUp);
        UpdateInputValue(ElemInputId_MouseAxisYPositive, 0, inputEvent, &inputActions->TouchRotateDown);
        UpdateInputValue(ElemInputId_TouchXNegative, 0, inputEvent, &inputActions->TouchRotateLeft);
        UpdateInputValue(ElemInputId_TouchXPositive, 0, inputEvent, &inputActions->TouchRotateRight);
        UpdateInputValue(ElemInputId_TouchYNegative, 0, inputEvent, &inputActions->TouchRotateUp);
        UpdateInputValue(ElemInputId_TouchYPositive, 0, inputEvent, &inputActions->TouchRotateDown);
        UpdateInputValue(ElemInputId_TouchXAbsolutePosition, 0, inputEvent, &inputActions->TouchPositionX);
        UpdateInputValue(ElemInputId_TouchYAbsolutePosition, 0, inputEvent, &inputActions->TouchPositionY);

        UpdateInputValue(ElemInputId_Touch, 1, inputEvent, &inputActions->Touch2);
        UpdateInputValueNegate(ElemInputId_Touch, 1, inputEvent, &inputActions->Touch2Released);
        UpdateInputValue(ElemInputId_TouchXNegative, 1, inputEvent, &inputActions->Touch2RotateLeft);
        UpdateInputValue(ElemInputId_TouchXPositive, 1, inputEvent, &inputActions->Touch2RotateRight);
        UpdateInputValue(ElemInputId_TouchYNegative, 1, inputEvent, &inputActions->Touch2RotateUp);
        UpdateInputValue(ElemInputId_TouchYPositive, 1, inputEvent, &inputActions->Touch2RotateDown);
        UpdateInputValue(ElemInputId_TouchXAbsolutePosition, 1, inputEvent, &inputActions->Touch2PositionX);
        UpdateInputValue(ElemInputId_TouchYAbsolutePosition, 1, inputEvent, &inputActions->Touch2PositionY);

        // TODO: For mouse wheel we should apply some scaling otherwise it is too slow
        UpdateInputValue(ElemInputId_KeyZ, 0, inputEvent, &inputActions->ZoomIn);
        UpdateInputValue(ElemInputID_GamepadRightShoulder, 0, inputEvent, &inputActions->ZoomIn);
        UpdateInputValue(ElemInputId_MouseWheelPositive, 0, inputEvent, &inputActions->ZoomIn);

        UpdateInputValue(ElemInputId_KeyX, 0, inputEvent, &inputActions->ZoomOut);
        UpdateInputValue(ElemInputID_GamepadLeftShoulder, 0, inputEvent, &inputActions->ZoomOut);
        UpdateInputValue(ElemInputId_MouseWheelNegative, 0, inputEvent, &inputActions->ZoomOut);

        UpdateInputValue(ElemInputId_KeySpacebar, 0, inputEvent, &inputActions->ChangeColor);
        UpdateInputValue(ElemInputId_MouseMiddleButton, 0, inputEvent, &inputActions->ChangeColor);
        UpdateInputValue(ElemInputID_GamepadButtonA, 0, inputEvent, &inputActions->ChangeColor);
        UpdateInputValue(ElemInputId_GamepadLeftStickButton, 0, inputEvent, &inputActions->ChangeColor);

        UpdateInputValue(ElemInputId_KeyF1, 0, inputEvent, &inputActions->HideCursor);
        UpdateInputValue(ElemInputId_KeyF2, 0, inputEvent, &inputActions->ShowCursor);
        UpdateInputValueNegate(ElemInputId_KeyEscape, 0, inputEvent, &inputActions->ExitApp);
        UpdateInputValueNegate(ElemInputId_MouseLeftButton, 0, inputEvent, &inputActions->TouchReleased);
        UpdateInputValueNegate(ElemInputId_MouseRightButton, 0, inputEvent, &inputActions->TouchRotateSideReleased);
        UpdateInputValueNegate(ElemInputId_Touch, 0, inputEvent, &inputActions->TouchReleased);
    }
}

// TODO: Put that in payload state
Vector3 rotationTouch = { };
float rotationTouchDecreaseSpeed = 0.001f;
float rotationTouchMaxSpeed = 0.3f;
Vector3 currentRotationSpeed = V3Zero;
float previousDistance = 0.0f;

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    // TODO: Double tap (color change)
    // TODO: Fix z rotation on touch? 

    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->ShaderParameters.AspectRatio = updateParameters->SwapChainInfo.AspectRatio;

    InputActions* inputActions = &applicationPayload->InputActions;
    UpdateInputs(inputActions);

    if (inputActions->ExitApp)
    {
        ElemExitApplication(0);
    }

    //ElemWindowCursorPosition cursorPosition = ElemGetWindowCursorPosition(applicationPayload->Window);
    //printf("Cursor Position: %u, %u\n", cursorPosition.X, cursorPosition.Y);

    Vector3 rotationDelta = V3Zero; 

    // TODO: Can we add options to normalize the speed we need for different path?
    if (inputActions->Touch && !inputActions->Touch2)
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

        if (MagnitudeSquaredV3(direction))
        {
            // TODO: The friction here is what we do for the touch. So maybe we can take the same code but with different constants?
            Vector3 acceleration = AddV3(MulScalarV3(direction, 500.0f), MulScalarV3(InverseV3(currentRotationSpeed), 60));

            rotationDelta = AddV3(MulScalarV3(acceleration, 0.5f * pow2f(updateParameters->DeltaTimeInSeconds)), MulScalarV3(currentRotationSpeed, updateParameters->DeltaTimeInSeconds));
            currentRotationSpeed = AddV3(MulScalarV3(acceleration, updateParameters->DeltaTimeInSeconds), currentRotationSpeed);

            printf("Current Speed: %f %f %f\n", currentRotationSpeed.X, currentRotationSpeed.Y, currentRotationSpeed.Z);
        }
    }

    if (inputActions->TouchRotateSide || inputActions->Touch2)
    {
        if (inputActions->Touch2)
        {
            // TODO: For rotation:
            // https://github.com/TouchScript/TouchScript/blob/master/Source/Assets/TouchScript/Scripts/Gestures/TransformGestures/ScreenTransformGesture.cs
            Vector2 touchDirection = (Vector2){ inputActions->TouchRotateLeft - inputActions->TouchRotateRight, inputActions->TouchRotateUp - inputActions->TouchRotateDown };
            Vector2 touch2Direction = (Vector2){ inputActions->Touch2RotateLeft - inputActions->Touch2RotateRight, inputActions->Touch2RotateUp - inputActions->Touch2RotateDown };

            printf("Dot Product: %f (V1 = %f, %f V2 = %f, %f)\n", DotProductV2(NormalizeV2(touchDirection), NormalizeV2(touch2Direction)), touchDirection.X, touchDirection.Y, touch2Direction.X, touch2Direction.Y); 

            if (DotProductV2(NormalizeV2(touchDirection), NormalizeV2(touch2Direction)) < -0.10f)
            {
                Vector2 touchPosition = (Vector2) { inputActions->TouchPositionX, inputActions->TouchPositionY };
                Vector2 touchPosition2 = (Vector2) { inputActions->Touch2PositionX, inputActions->Touch2PositionY };
                float distance = MagnitudeV2(SubstractV2(touchPosition, touchPosition2));

                printf("Zoom: %f\n", distance > previousDistance ? -1.0f : 1.0f);

                // TODO: Can we compute the strength based on the position?
                float strength = fmaxf(MagnitudeV2(touchDirection), MagnitudeV2(touch2Direction));
                applicationPayload->ShaderParameters.Zoom += (distance < previousDistance ? -1.0f : 1.0f) * strength * 10.0f * updateParameters->DeltaTimeInSeconds;
                previousDistance = distance;
            }
            else
            {
                rotationDelta.Z = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * 2.0f * updateParameters->DeltaTimeInSeconds;
            }
        }
        else
        {
            rotationDelta.Z = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * 2.0f * updateParameters->DeltaTimeInSeconds;
        }
    }
    else
    {
        rotationDelta.Z = (inputActions->RotateSideLeft - inputActions->RotateSideRight) * 5.0f * updateParameters->DeltaTimeInSeconds;
    }

    if (inputActions->TouchReleased || inputActions->TouchRotateSideReleased || inputActions->Touch2Released)
    {
        // TODO: For the moment it is impossible to pull of on iphone because there is small delay between the 2 releases
        if (inputActions->TouchReleased && !inputActions->Touch2Released)
        {
            rotationTouch.X = (inputActions->TouchRotateUp - inputActions->TouchRotateDown) * 4.0f * updateParameters->DeltaTimeInSeconds;
            rotationTouch.Y = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * 4.0f * updateParameters->DeltaTimeInSeconds;
        }

        if (inputActions->TouchRotateSideReleased || inputActions->Touch2Released)
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

