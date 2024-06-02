#include <math.h>
#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"
#include "SampleInputs.h"

#define ROTATION_TOUCH_DECREASE_SPEED 0.001f
#define ROTATION_TOUCH_SPEED 4.0f
#define ROTATION_TOUCH_MAX_DELTA 0.3f
#define ROTATION_MULTITOUCH_SPEED 200.0f
#define ROTATION_ACCELERATION 500.0f
#define ROTATION_FRICTION 60.0f
#define ZOOM_MULTITOUCH_SPEED 1000.0f
#define ZOOM_SPEED 5.0f

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
    uint32_t RenderTextureIndex;
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
    ElemGraphicsHeap GraphicsHeap;
    ElemTexture RenderTexture;
    ElemShaderDescriptor RenderTextureUavDescriptor;
    ElemSwapChain SwapChain;
    ElemPipelineState GraphicsPipeline;
    ShaderParameters ShaderParameters;
    InputActions InputActions;
    InputActionBindingSpan InputActionBindings;
    GameState GameState;
} ApplicationPayload;
    
void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

void RegisterInputBindings(ApplicationPayload* applicationPayload)
{
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyA, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateLeft);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyD, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateRight);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyW, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateUp);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyS, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateDown);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyQ, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideLeft);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyE, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideRight);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyZ, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomIn);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyX, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomOut);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeySpacebar, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TriangleColor);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyF1, 0, InputActionBindingType_ReleasedSwitch, &applicationPayload->InputActions.ShowCursor);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyEscape, 0, InputActionBindingType_Released, &applicationPayload->InputActions.ExitApp);

    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseLeftButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.Touch);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseLeftButton, 0, InputActionBindingType_DoubleReleasedSwitch, &applicationPayload->InputActions.TriangleColor);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseLeftButton, 0, InputActionBindingType_Released, &applicationPayload->InputActions.TouchReleased);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseRightButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateSide);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseAxisXNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateLeft);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseAxisXPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateRight);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseAxisYNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateUp);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseAxisYPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateDown);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseWheelPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomIn);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseWheelNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomOut);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseMiddleButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TriangleColor);

    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_GamepadLeftStickXNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateLeft);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_GamepadLeftStickXPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateRight);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_GamepadLeftStickYPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateUp);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_GamepadLeftStickYNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateDown);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_GamepadLeftStickButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TriangleColor);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputID_GamepadLeftTrigger, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideLeft);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputID_GamepadRightTrigger, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideRight);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputID_GamepadLeftShoulder, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomOut);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputID_GamepadRightShoulder, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomIn);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputID_GamepadButtonA, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TriangleColor);

    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_Touch, 0, InputActionBindingType_Value, &applicationPayload->InputActions.Touch);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchXNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateLeft);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchXPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateRight);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchYNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateUp);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchYPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchRotateDown);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchXAbsolutePosition, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchPositionX);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchYAbsolutePosition, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchPositionY);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_Touch, 1, InputActionBindingType_Value, &applicationPayload->InputActions.Touch2);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchXAbsolutePosition, 1, InputActionBindingType_Value, &applicationPayload->InputActions.Touch2PositionX);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchYAbsolutePosition, 1, InputActionBindingType_Value, &applicationPayload->InputActions.Touch2PositionY);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_Touch, 0, InputActionBindingType_Released, &applicationPayload->InputActions.TouchReleased);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_Touch, 0, InputActionBindingType_DoubleReleasedSwitch, &applicationPayload->InputActions.TriangleColor);
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

    applicationPayload->GraphicsHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64), NULL);
    
    applicationPayload->RenderTexture = ElemCreateTexture(applicationPayload->GraphicsHeap, 0, &(ElemTextureParameters)
    {
        .Width = 256,
        .Height = 256,
        .Format = ElemTextureFormat_B8G8R8A8_UNORM,
        .Usage = ElemTextureUsage_Uav,
        .DebugName = "Render Texture"
    });

    applicationPayload->RenderTextureUavDescriptor = ElemCreateTextureShaderDescriptor(applicationPayload->RenderTexture, &(ElemTextureShaderDescriptorOptions) { .Type = ElemTextureShaderDescriptorType_Uav });

    ElemDataSpan shaderData = SampleReadFile(!applicationPayload->PreferVulkan ? "Triangle.shader": "Triangle_vulkan.shader");
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, shaderData);

    // TODO: Compute pipeline creation

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
    ElemFreeTexture(applicationPayload->RenderTexture);
    ElemFreeGraphicsHeap(applicationPayload->GraphicsHeap);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);
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
    applicationPayload->ShaderParameters.RenderTextureIndex = applicationPayload->RenderTextureUavDescriptor;

    InputActions* inputActions = &applicationPayload->InputActions;
    SampleUpdateInputActions(&applicationPayload->InputActionBindings);

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

    ElemBindGraphicsHeap(commandList, applicationPayload->GraphicsHeap);
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
        SampleSetWindowTitle(applicationPayload->Window, "HelloCompute", applicationPayload->GraphicsDevice, frameMeasurement.FrameTimeInSeconds, frameMeasurement.Fps);
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

