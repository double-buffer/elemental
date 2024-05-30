#include <math.h>
#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"

typedef enum
{
    InputActionBindingType_Value,
    InputActionBindingType_Released,
    InputActionBindingType_DoubleReleased,
} InputActionBindingType;

typedef struct
{
    ElemInputId InputId;
    InputActionBindingType BindingType;
    uint32_t Index;
    float* ActionValue;
} InputActionBinding;

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
    float TouchRotateLeft;
    float TouchRotateRight;
    float TouchRotateUp;
    float TouchRotateDown;
    float TouchPositionX;
    float TouchPositionY;

    float Touch2;
    float Touch2PositionX;
    float Touch2PositionY;
    
    float TouchRotateSide;

    float ZoomIn;
    float ZoomOut;
    float ChangeColor;
    float HideCursor;
    float ShowCursor;

    float ExitApp;

    uint32_t TouchReleasedCount;
    double LastTouchReleasedTime;
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
    ShaderParameters ShaderParameters;
    InputActions InputActions;
    InputActionBinding InputActionBindings[255];
    uint32_t InputActionBindingCount;
} ApplicationPayload;
    
void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

void RegisterInputActionBinding(ApplicationPayload* applicationPayload, ElemInputId inputId, uint32_t index, InputActionBindingType bindingType, float* actionValue)
{
    applicationPayload->InputActionBindings[applicationPayload->InputActionBindingCount++] = (InputActionBinding)
    { 
        .InputId = inputId, 
        .BindingType = bindingType,
        .Index = index,
        .ActionValue = actionValue
    };
}

void InitInputBindings(ApplicationPayload* applicationPayload)
{
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyA, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateLeft);
    RegisterInputActionBinding(applicationPayload, ElemInputId_GamepadLeftStickXNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateLeft);

    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyD, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateRight);
    RegisterInputActionBinding(applicationPayload, ElemInputId_GamepadLeftStickXPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateRight);
    
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyW, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateUp);
    RegisterInputActionBinding(applicationPayload, ElemInputId_GamepadLeftStickYPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateUp);

    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyS, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateDown);
    RegisterInputActionBinding(applicationPayload, ElemInputId_GamepadLeftStickYNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateDown);

    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyQ, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideLeft);
    RegisterInputActionBinding(applicationPayload, ElemInputID_GamepadLeftTrigger, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideLeft);

    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyE, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideRight);
    RegisterInputActionBinding(applicationPayload, ElemInputID_GamepadRightTrigger, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideRight);

    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseLeftButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.Touch);
    RegisterInputActionBinding(applicationPayload, ElemInputId_Touch, 0, InputActionBindingType_Value, &applicationPayload->InputActions.Touch);

    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseRightButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateSide);

    // TODO: Apply some pre multipliers
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseAxisXNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateLeft);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchXNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateLeft);

    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseAxisXPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateRight);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchXPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateRight);

    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseAxisYNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateUp);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchYNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateUp);

    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseAxisYPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateDown);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchYPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateDown);

    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchXAbsolutePosition, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchPositionX);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchYAbsolutePosition, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchPositionY);

    RegisterInputActionBinding(applicationPayload, ElemInputId_Touch, 1, InputActionBindingType_Value, &applicationPayload->InputActions.Touch2);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchXAbsolutePosition, 1, InputActionBindingType_Value, &applicationPayload->InputActions.Touch2PositionX);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchYAbsolutePosition, 1, InputActionBindingType_Value, &applicationPayload->InputActions.Touch2PositionY);

    // TODO: For mouse wheel we should apply some scaling otherwise it is too slow
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyZ, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomIn);
    RegisterInputActionBinding(applicationPayload, ElemInputID_GamepadRightShoulder, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomIn);
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseWheelPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomIn);

    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyX, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomOut);
    RegisterInputActionBinding(applicationPayload, ElemInputID_GamepadLeftShoulder, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomOut);
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseWheelNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomOut);

    RegisterInputActionBinding(applicationPayload, ElemInputId_KeySpacebar, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ChangeColor);
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseMiddleButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ChangeColor);
    RegisterInputActionBinding(applicationPayload, ElemInputID_GamepadButtonA, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ChangeColor);
    RegisterInputActionBinding(applicationPayload, ElemInputId_GamepadLeftStickButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ChangeColor);

    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyF1, 0, InputActionBindingType_Value, &applicationPayload->InputActions.HideCursor);
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyF2, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ShowCursor);

    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyEscape, 0, InputActionBindingType_Released, &applicationPayload->InputActions.ExitApp);

    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseLeftButton, 0, InputActionBindingType_Released, &applicationPayload->InputActions.TouchReleased);
    RegisterInputActionBinding(applicationPayload, ElemInputId_Touch, 0, InputActionBindingType_Released, &applicationPayload->InputActions.TouchReleased);
}

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

    InitInputBindings(applicationPayload);
    
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

void UpdateInputs(InputActions* inputActions, InputActionBinding* inputActionBindings, uint32_t inputActionBindingCount)
{
    ElemInputStream inputStream = ElemGetInputStream();

    for (uint32_t i = 0; i < inputActionBindingCount; i++)
    {
        InputActionBinding binding = inputActionBindings[i];

        if (binding.BindingType == InputActionBindingType_Released)
        {
            *binding.ActionValue = 0.0f;
        }
    }

    for (uint32_t i = 0; i < inputStream.Events.Length; i++)
    {
        ElemInputEvent* inputEvent = &inputStream.Events.Items[i];

        if (inputEvent->InputType != ElemInputType_Delta && inputEvent->InputType != ElemInputType_Absolute)
        {
            printf("Received an input event %d: Device=%lu, Value=%f (Elapsed: %f)\n", inputEvent->InputId, inputEvent->InputDevice, inputEvent->Value, inputEvent->ElapsedSeconds);
        }

        for (uint32_t j = 0; j < inputActionBindingCount; j++)
        {
            InputActionBinding binding = inputActionBindings[j];

            if (binding.BindingType == InputActionBindingType_Value && 
                inputEvent->InputId == binding.InputId && 
                inputEvent->InputDeviceTypeIndex == binding.Index)
            {
                *binding.ActionValue = inputEvent->Value;
            }
            else if (binding.BindingType == InputActionBindingType_Released && 
                     inputEvent->InputId == binding.InputId && 
                     inputEvent->InputDeviceTypeIndex == binding.Index)
            {
                *binding.ActionValue = !inputEvent->Value;
            }
        }


        if (((inputEvent->InputId == ElemInputId_Touch && inputEvent->InputDeviceTypeIndex == 0) || inputEvent->InputId == ElemInputId_MouseLeftButton) && inputEvent->Value == 0)
        {
            if ((inputEvent->ElapsedSeconds - inputActions->LastTouchReleasedTime) > 0.25f)
            {
                inputActions->TouchReleasedCount = 1;
            }
            else
            {
                inputActions->TouchReleasedCount++;

                if (inputActions->TouchReleasedCount > 1)
                {
                    inputActions->ChangeColor = !inputActions->ChangeColor;
                    inputActions->TouchReleasedCount = 0;
                }
            }

            inputActions->LastTouchReleasedTime = inputEvent->ElapsedSeconds;
        }
    }
}

float rotationTouchDecreaseSpeed = 0.001f;
float rotationTouchMaxSpeed = 0.3f;

// TODO: Put that in payload state
Vector2 rotationTouch = { };
Vector3 currentRotationSpeed = V3Zero;
float previousDistance = 0.0f;
float previousAngle = 0.0f;

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    // TODO: Double tap (color change)

    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->ShaderParameters.AspectRatio = updateParameters->SwapChainInfo.AspectRatio;

    InputActions* inputActions = &applicationPayload->InputActions;
    UpdateInputs(inputActions, applicationPayload->InputActionBindings, applicationPayload->InputActionBindingCount);

    if (inputActions->ExitApp)
    {
        ElemExitApplication(0);
    }

    Vector3 rotationDelta = V3Zero; 

    // TODO: Review constant values
    if (inputActions->Touch)
    {
        if (inputActions->Touch2)
        {
            Vector2 touchPosition = (Vector2) { inputActions->TouchPositionX, inputActions->TouchPositionY };
            Vector2 touchPosition2 = (Vector2) { inputActions->Touch2PositionX, inputActions->Touch2PositionY };

            Vector2 diffVector = SubstractV2(touchPosition, touchPosition2);
            float distance = MagnitudeV2(diffVector);
            float angle = atan2(diffVector.X, diffVector.Y);

            float zoom = distance - previousDistance;

            if (previousDistance != 0.0f)
            {
                applicationPayload->ShaderParameters.Zoom += zoom * 1000.0f * updateParameters->DeltaTimeInSeconds;
            }

            float rotation = NormalizeAngle(angle - previousAngle);

            if (previousAngle != 0.0f)
            {
                rotationDelta.Z = -rotation * 200.0f * updateParameters->DeltaTimeInSeconds;
            }

            previousDistance = distance;
            previousAngle = angle;
        }
        else 
        {
            rotationTouch = V2Zero;
            previousDistance = 0.0f;
            previousAngle = 0.0f;   

            rotationDelta.X = (inputActions->TouchRotateUp - inputActions->TouchRotateDown) * 4.0f * updateParameters->DeltaTimeInSeconds;
            rotationDelta.Y = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * 4.0f * updateParameters->DeltaTimeInSeconds;
        }
    }
    else if (inputActions->TouchRotateSide)
    {
        rotationTouch = V2Zero;
        previousDistance = 0.0f;
        previousAngle = 0.0f;   

        rotationDelta.Z = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * 2.0f * updateParameters->DeltaTimeInSeconds;
    }
    else if (inputActions->TouchReleased && !inputActions->Touch2)
    {
        rotationTouch = V2Zero;
        previousDistance = 0.0f;
        previousAngle = 0.0f;   

        rotationTouch.X = (inputActions->TouchRotateUp - inputActions->TouchRotateDown) * 4.0f * updateParameters->DeltaTimeInSeconds;
        rotationTouch.Y = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * 4.0f * updateParameters->DeltaTimeInSeconds;
    }
    else
    {
        Vector3 direction = NormalizeV3((Vector3) 
        { 
            .X = (inputActions->RotateUp - inputActions->RotateDown),
            .Y = (inputActions->RotateLeft - inputActions->RotateRight),
            .Z = (inputActions->RotateSideLeft - inputActions->RotateSideRight)
        });

        if (MagnitudeSquaredV3(direction))
        {
            // TODO: The friction here is what we do for the touch. So maybe we can take the same code but with different constants?
            Vector3 acceleration = AddV3(MulScalarV3(direction, 500.0f), MulScalarV3(InverseV3(currentRotationSpeed), 60));

            rotationTouch = V2Zero;
            rotationDelta = AddV3(MulScalarV3(acceleration, 0.5f * pow2f(updateParameters->DeltaTimeInSeconds)), MulScalarV3(currentRotationSpeed, updateParameters->DeltaTimeInSeconds));
            currentRotationSpeed = AddV3(MulScalarV3(acceleration, updateParameters->DeltaTimeInSeconds), currentRotationSpeed);
            //printf("Current Speed: %f %f %f\n", currentRotationSpeed.X, currentRotationSpeed.Y, currentRotationSpeed.Z);
        }
    }

    if (MagnitudeSquaredV2(rotationTouch) > 0)
    {
        if (MagnitudeV2(rotationTouch) > rotationTouchMaxSpeed)
        {
            rotationTouch = MulScalarV2(NormalizeV2(rotationTouch), rotationTouchMaxSpeed);
        }

        //printf("Touch Speed = %f, %f, %f (%f)\n", rotationTouch.X, rotationTouch.Y, rotationTouch.Z, MagnitudeV3(rotationTouch));
        rotationDelta = AddV3(rotationDelta, (Vector3){ rotationTouch.X, rotationTouch.Y, 0.0f });

        Vector2 inverse = MulScalarV2(NormalizeV2(InverseV2(rotationTouch)), rotationTouchDecreaseSpeed);
        rotationTouch = AddV2(rotationTouch, inverse);

        if (MagnitudeV2(rotationTouch) < 0.001f)
        {
            rotationTouch = V2Zero;
        }
    }
        
    //printf("Rotation Delta = %f, %f, %f, (%f)\n", rotationDelta.X, rotationDelta.Y, rotationDelta.Z, inputActions->TouchRotateUp);

    if (MagnitudeSquaredV3(rotationDelta))
    {
        Vector4 rotationQuaternion = MulQuat(CreateQuaternion((Vector3){ 1, 0, 0 }, rotationDelta.X), MulQuat(CreateQuaternion((Vector3){ 0, 0, 1 }, rotationDelta.Z), CreateQuaternion((Vector3){ 0, 1, 0 }, rotationDelta.Y)));
        applicationPayload->ShaderParameters.RotationQuaternion = MulQuat(rotationQuaternion, applicationPayload->ShaderParameters.RotationQuaternion);
    }

    float maxZoom = (applicationPayload->ShaderParameters.AspectRatio >= 0.75 ? 1.5f : 3.5f);
    applicationPayload->ShaderParameters.Zoom = fminf(maxZoom, (inputActions->ZoomIn - inputActions->ZoomOut) * 5.0f * updateParameters->DeltaTimeInSeconds + applicationPayload->ShaderParameters.Zoom);
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

