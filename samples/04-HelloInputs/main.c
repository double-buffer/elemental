#include <math.h>
#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"

#define ROTATION_TOUCH_DECREASE_SPEED 0.001f
#define ROTATION_TOUCH_SPEED 4.0f
#define ROTATION_TOUCH_MAX_DELTA 0.3f
#define ROTATION_MULTITOUCH_SPEED 200.0f
#define ROTATION_ACCELERATION 500.0f
#define ROTATION_FRICTION 60.0f
#define ZOOM_MULTITOUCH_SPEED 1000.0f
#define ZOOM_SPEED 5.0f

typedef enum
{
    InputActionBindingType_Value,
    InputActionBindingType_Released,
    InputActionBindingType_ReleasedSwitch,
    InputActionBindingType_DoubleReleasedSwitch,
} InputActionBindingType;

typedef struct
{
    ElemInputId InputId;
    InputActionBindingType BindingType;
    uint32_t Index;
    float* ActionValue;
    uint32_t ReleasedCount;
    double LastReleasedTime;
} InputActionBinding;

typedef struct
{
    float RotateLeft;
    float RotateRight;
    float RotateUp;
    float RotateDown;
    float RotateSideLeft;
    float RotateSideRight;
    float ZoomIn;
    float ZoomOut;

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
    float TriangleColor;
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
    Vector3 RotationDelta;
    Vector2 RotationTouch;
    Vector3 CurrentRotationSpeed;
    float PreviousTouchDistance;
    float PreviousTouchAngle;
    float Zoom;
} GameState;

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
    GameState GameState;
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

void RegisterInputBindings(ApplicationPayload* applicationPayload)
{
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyA, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateLeft);
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyD, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateRight);
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyW, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateUp);
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyS, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateDown);
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyQ, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideLeft);
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyE, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideRight);
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyZ, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomIn);
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyX, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomOut);
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeySpacebar, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TriangleColor);
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyF1, 0, InputActionBindingType_ReleasedSwitch, &applicationPayload->InputActions.ShowCursor);
    RegisterInputActionBinding(applicationPayload, ElemInputId_KeyEscape, 0, InputActionBindingType_Released, &applicationPayload->InputActions.ExitApp);

    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseLeftButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.Touch);
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseLeftButton, 0, InputActionBindingType_DoubleReleasedSwitch, &applicationPayload->InputActions.TriangleColor);
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseLeftButton, 0, InputActionBindingType_Released, &applicationPayload->InputActions.TouchReleased);
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseRightButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateSide);
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseAxisXNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateLeft);
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseAxisXPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateRight);
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseAxisYNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateUp);
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseAxisYPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateDown);
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseWheelPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomIn);
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseWheelNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomOut);
    RegisterInputActionBinding(applicationPayload, ElemInputId_MouseMiddleButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TriangleColor);

    RegisterInputActionBinding(applicationPayload, ElemInputId_GamepadLeftStickXNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateLeft);
    RegisterInputActionBinding(applicationPayload, ElemInputId_GamepadLeftStickXPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateRight);
    RegisterInputActionBinding(applicationPayload, ElemInputId_GamepadLeftStickYPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateUp);
    RegisterInputActionBinding(applicationPayload, ElemInputId_GamepadLeftStickYNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateDown);
    RegisterInputActionBinding(applicationPayload, ElemInputId_GamepadLeftStickButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TriangleColor);
    RegisterInputActionBinding(applicationPayload, ElemInputID_GamepadLeftTrigger, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideLeft);
    RegisterInputActionBinding(applicationPayload, ElemInputID_GamepadRightTrigger, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideRight);
    RegisterInputActionBinding(applicationPayload, ElemInputID_GamepadLeftShoulder, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomOut);
    RegisterInputActionBinding(applicationPayload, ElemInputID_GamepadRightShoulder, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomIn);
    RegisterInputActionBinding(applicationPayload, ElemInputID_GamepadButtonA, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TriangleColor);

    RegisterInputActionBinding(applicationPayload, ElemInputId_Touch, 0, InputActionBindingType_Value, &applicationPayload->InputActions.Touch);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchXNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateLeft);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchXPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateRight);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchYNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateUp);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchYPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateDown);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchXAbsolutePosition, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchPositionX);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchYAbsolutePosition, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchPositionY);
    RegisterInputActionBinding(applicationPayload, ElemInputId_Touch, 1, InputActionBindingType_Value, &applicationPayload->InputActions.Touch2);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchXAbsolutePosition, 1, InputActionBindingType_Value, &applicationPayload->InputActions.Touch2PositionX);
    RegisterInputActionBinding(applicationPayload, ElemInputId_TouchYAbsolutePosition, 1, InputActionBindingType_Value, &applicationPayload->InputActions.Touch2PositionY);
    RegisterInputActionBinding(applicationPayload, ElemInputId_Touch, 0, InputActionBindingType_Released, &applicationPayload->InputActions.TouchReleased);
    RegisterInputActionBinding(applicationPayload, ElemInputId_Touch, 0, InputActionBindingType_DoubleReleasedSwitch, &applicationPayload->InputActions.TriangleColor);
}

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
    applicationPayload->InputActions.ShowCursor = true;

    RegisterInputBindings(applicationPayload);
    
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

void UpdateInputActions(InputActions* inputActions, InputActionBinding* inputActionBindings, uint32_t inputActionBindingCount)
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

        for (uint32_t j = 0; j < inputActionBindingCount; j++)
        {
            InputActionBinding* binding = &inputActionBindings[j];

            if (inputEvent->InputId == binding->InputId && inputEvent->InputDeviceTypeIndex == binding->Index)
            {
                if (binding->BindingType == InputActionBindingType_Value)
                {
                    *binding->ActionValue = inputEvent->Value;
                }
                else if (binding->BindingType == InputActionBindingType_Released)
                {
                    *binding->ActionValue = !inputEvent->Value;
                }
                else if (binding->BindingType == InputActionBindingType_ReleasedSwitch && inputEvent->Value == 0.0f)
                {
                    *binding->ActionValue = !*binding->ActionValue;
                }
                else if (binding->BindingType == InputActionBindingType_DoubleReleasedSwitch && inputEvent->Value == 0.0f)
                {
                    if ((inputEvent->ElapsedSeconds - binding->LastReleasedTime) > 0.25f)
                    {
                        binding->ReleasedCount = 1;
                    }
                    else
                    {
                        binding->ReleasedCount++;

                        if (binding->ReleasedCount > 1)
                        {
                            *binding->ActionValue = !*binding->ActionValue;
                            binding->ReleasedCount = 0;
                        }
                    }

                    binding->LastReleasedTime = inputEvent->ElapsedSeconds;
                }
            }
        }
    }
}

void ResetTouchParameters(GameState* gameState) 
{
    gameState->RotationTouch = V2Zero;
    gameState->PreviousTouchDistance = 0.0f;
    gameState->PreviousTouchAngle = 0.0f;   
}

void UpdateGameState(GameState* gameState, InputActions* inputActions, float deltaTimeInSeconds)
{
    gameState->RotationDelta = V3Zero; 

    if (inputActions->Touch)
    {
        if (inputActions->Touch2)
        {
            Vector2 touchPosition = (Vector2) { inputActions->TouchPositionX, inputActions->TouchPositionY };
            Vector2 touchPosition2 = (Vector2) { inputActions->Touch2PositionX, inputActions->Touch2PositionY };

            Vector2 diffVector = SubstractV2(touchPosition, touchPosition2);
            float distance = MagnitudeV2(diffVector);
            float angle = atan2(diffVector.X, diffVector.Y);

            if (gameState->PreviousTouchDistance != 0.0f)
            {
                gameState->Zoom += (distance - gameState->PreviousTouchDistance) * ZOOM_MULTITOUCH_SPEED * deltaTimeInSeconds;
            }

            if (gameState->PreviousTouchAngle != 0.0f)
            {
                gameState->RotationDelta.Z = -NormalizeAngle(angle - gameState->PreviousTouchAngle) * ROTATION_MULTITOUCH_SPEED * deltaTimeInSeconds;
            }

            gameState->PreviousTouchDistance = distance;
            gameState->PreviousTouchAngle = angle;
        }
        else 
        {
            ResetTouchParameters(gameState);

            gameState->RotationDelta.X = (inputActions->TouchRotateUp - inputActions->TouchRotateDown) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
            gameState->RotationDelta.Y = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
        }
    }
    else if (inputActions->TouchRotateSide)
    {
        ResetTouchParameters(gameState);
        gameState->RotationDelta.Z = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
    }
    else if (inputActions->TouchReleased && !inputActions->Touch2)
    {
        ResetTouchParameters(gameState);

        gameState->RotationTouch.X = (inputActions->TouchRotateUp - inputActions->TouchRotateDown) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
        gameState->RotationTouch.Y = (inputActions->TouchRotateLeft - inputActions->TouchRotateRight) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
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
            Vector3 acceleration = AddV3(MulScalarV3(direction, ROTATION_ACCELERATION), MulScalarV3(InverseV3(gameState->CurrentRotationSpeed), ROTATION_FRICTION));

            ResetTouchParameters(gameState);
            gameState->RotationDelta = AddV3(MulScalarV3(acceleration, 0.5f * pow2f(deltaTimeInSeconds)), MulScalarV3(gameState->CurrentRotationSpeed, deltaTimeInSeconds));
            gameState->CurrentRotationSpeed = AddV3(MulScalarV3(acceleration, deltaTimeInSeconds), gameState->CurrentRotationSpeed);
        }
    }

    if (MagnitudeSquaredV2(gameState->RotationTouch) > 0)
    {
        if (MagnitudeV2(gameState->RotationTouch) > ROTATION_TOUCH_MAX_DELTA)
        {
            gameState->RotationTouch = MulScalarV2(NormalizeV2(gameState->RotationTouch), ROTATION_TOUCH_MAX_DELTA);
        }

        gameState->RotationDelta = AddV3(gameState->RotationDelta, (Vector3){ gameState->RotationTouch.X, gameState->RotationTouch.Y, 0.0f });

        Vector2 inverse = MulScalarV2(NormalizeV2(InverseV2(gameState->RotationTouch)), ROTATION_TOUCH_DECREASE_SPEED);
        gameState->RotationTouch = AddV2(gameState->RotationTouch, inverse);

        if (MagnitudeV2(gameState->RotationTouch) < 0.001f)
        {
            ResetTouchParameters(gameState);
        }
    }

    gameState->Zoom += (inputActions->ZoomIn - inputActions->ZoomOut) * ZOOM_SPEED * deltaTimeInSeconds; 
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->ShaderParameters.AspectRatio = updateParameters->SwapChainInfo.AspectRatio;

    InputActions* inputActions = &applicationPayload->InputActions;
    UpdateInputActions(inputActions, applicationPayload->InputActionBindings, applicationPayload->InputActionBindingCount);

    if (inputActions->ExitApp)
    {
        ElemExitApplication(0);
    }

    if (inputActions->ShowCursor)
    {
        ElemShowWindowCursor(applicationPayload->Window);
    }
    else
    {
        ElemHideWindowCursor(applicationPayload->Window);
    }

    GameState* gameState = &applicationPayload->GameState;
    UpdateGameState(gameState, inputActions, updateParameters->DeltaTimeInSeconds);

    if (MagnitudeSquaredV3(gameState->RotationDelta))
    {
        Vector4 rotationQuaternion = MulQuat(CreateQuaternion((Vector3){ 1, 0, 0 }, gameState->RotationDelta.X), MulQuat(CreateQuaternion((Vector3){ 0, 0, 1 }, gameState->RotationDelta.Z), CreateQuaternion((Vector3){ 0, 1, 0 }, gameState->RotationDelta.Y)));
        applicationPayload->ShaderParameters.RotationQuaternion = MulQuat(rotationQuaternion, applicationPayload->ShaderParameters.RotationQuaternion);
    }

    float maxZoom = (applicationPayload->ShaderParameters.AspectRatio >= 0.75 ? 1.5f : 3.5f);
    applicationPayload->ShaderParameters.Zoom = fminf(maxZoom, gameState->Zoom);
    applicationPayload->ShaderParameters.TriangeColor = inputActions->TriangleColor;

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

    if (frameMeasurement.HasNewData)
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

