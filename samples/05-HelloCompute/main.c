#include <math.h>
#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"
#include "SampleInputs.h"

#define ROTATION_TOUCH_DECREASE_SPEED 0.001f
#define ROTATION_TOUCH_SPEED 1.0f
#define ROTATION_TOUCH_MAX_DELTA 0.3f
#define ROTATION_MULTITOUCH_SPEED 200.0f
#define ROTATION_ACCELERATION 50.0f
#define ROTATION_FRICTION 40.0f
#define ZOOM_MULTITOUCH_SPEED 1000.0f
#define ZOOM_SPEED 0.5f
#define ZOOM_FACTOR 10.0f

typedef struct
{
    float TranslateLeft;
    float TranslateRight;
    float TranslateUp;
    float TranslateDown;
    float RotateSideLeft;
    float RotateSideRight;
    float ZoomIn;
    float ZoomOut;

    float Touch;
    float TouchReleased;
    float TouchTranslateLeft;
    float TouchTranslateRight;
    float TouchTranslateUp;
    float TouchTranslateDown;
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
    uint32_t RenderTextureIndex;
    uint32_t TriangeColor;
    float Zoom;
    float Reserved;
    Matrix3x3 Transform;
} ShaderParameters;

typedef struct
{
    Vector2 TranslationDelta;
    float RotationDelta;
    Vector2 RotationTouch;
    Vector2 CurrentTranslationSpeed;
    float PreviousTouchDistance;
    float PreviousTouchAngle;
    float Zoom;
} GameState;

// TODO: Group common variables into separate structs
typedef struct
{
    bool PreferVulkan;
    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    ElemGraphicsHeap GraphicsHeap;
    uint32_t CurrentHeapOffset;
    ElemTexture RenderTexture;
    ElemShaderDescriptor RenderTextureUavDescriptor;
    ElemShaderDescriptor RenderTextureReadDescriptor;
    ElemSwapChain SwapChain;
    ElemPipelineState GraphicsPipeline;
    ElemPipelineState ComputePipeline;
    ShaderParameters ShaderParameters;
    InputActions InputActions;
    InputActionBindingSpan InputActionBindings;
    GameState GameState;
} ApplicationPayload;
    
void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

void RegisterInputBindings(ApplicationPayload* applicationPayload)
{
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyA, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TranslateLeft);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyD, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TranslateRight);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyW, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TranslateUp);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_KeyS, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TranslateDown);
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
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseAxisXPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchTranslateLeft);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseAxisXNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchTranslateRight);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseAxisYPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchTranslateUp);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseAxisYNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchTranslateDown);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseWheelPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomIn);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseWheelNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomOut);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_MouseMiddleButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TriangleColor);

    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_GamepadLeftStickXNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TranslateLeft);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_GamepadLeftStickXPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TranslateRight);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_GamepadLeftStickYPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TranslateUp);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_GamepadLeftStickYNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TranslateDown);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_GamepadLeftStickButton, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TriangleColor);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputID_GamepadLeftTrigger, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideLeft);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputID_GamepadRightTrigger, 0, InputActionBindingType_Value, &applicationPayload->InputActions.RotateSideRight);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputID_GamepadLeftShoulder, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomOut);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputID_GamepadRightShoulder, 0, InputActionBindingType_Value, &applicationPayload->InputActions.ZoomIn);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputID_GamepadButtonA, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TriangleColor);

    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_Touch, 0, InputActionBindingType_Value, &applicationPayload->InputActions.Touch);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchXNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchTranslateLeft);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchXPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchTranslateRight);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchYNegative, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchTranslateUp);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchYPositive, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchTranslateDown);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchXAbsolutePosition, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchPositionX);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchYAbsolutePosition, 0, InputActionBindingType_Value, &applicationPayload->InputActions.TouchPositionY);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_Touch, 1, InputActionBindingType_Value, &applicationPayload->InputActions.Touch2);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchXAbsolutePosition, 1, InputActionBindingType_Value, &applicationPayload->InputActions.Touch2PositionX);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_TouchYAbsolutePosition, 1, InputActionBindingType_Value, &applicationPayload->InputActions.Touch2PositionY);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_Touch, 0, InputActionBindingType_Released, &applicationPayload->InputActions.TouchReleased);
    SampleRegisterInputActionBinding(&applicationPayload->InputActionBindings, ElemInputId_Touch, 0, InputActionBindingType_DoubleReleasedSwitch, &applicationPayload->InputActions.TriangleColor);
}

void CreateRenderTexture(ApplicationPayload* applicationPayload, uint32_t width, uint32_t height)
{
    // TODO: Delete texture descriptors
    // TODO: Delete texture
    // TODO: Test Delete texture with latency=2 (Check driver settings because it seems nvidia is overriding it :()

    // TODO: Get Size requirements

    printf("Creating render texture...\n");

    applicationPayload->RenderTexture = ElemCreateTexture(applicationPayload->GraphicsHeap, 0, &(ElemTextureParameters)
    {
        .Width = width,
        .Height = height,
        .Format = ElemTextureFormat_R16G16B16A16_FLOAT,
        .Usage = ElemTextureUsage_Uav,
        .DebugName = "Render Texture"
    });

    applicationPayload->RenderTextureUavDescriptor = ElemCreateTextureShaderDescriptor(applicationPayload->RenderTexture, &(ElemTextureShaderDescriptorOptions) { .Type = ElemTextureShaderDescriptorType_Uav });
    applicationPayload->RenderTextureReadDescriptor = ElemCreateTextureShaderDescriptor(applicationPayload->RenderTexture, &(ElemTextureShaderDescriptorOptions) { .Type = ElemTextureShaderDescriptorType_Read });

    // TODO: Increment offset
    // TODO: Reuse memory?
}

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->Window = ElemCreateWindow(NULL);

    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = true, .PreferVulkan = applicationPayload->PreferVulkan });
    applicationPayload->GraphicsDevice = ElemCreateGraphicsDevice(NULL);

    applicationPayload->CommandQueue= ElemCreateCommandQueue(applicationPayload->GraphicsDevice, ElemCommandQueueType_Graphics, NULL);
    applicationPayload->SwapChain= ElemCreateSwapChain(applicationPayload->CommandQueue, applicationPayload->Window, UpdateSwapChain, &(ElemSwapChainOptions) {.FrameLatency = 1, .UpdatePayload = payload });
    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(applicationPayload->SwapChain);

    applicationPayload->GraphicsHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64), NULL);
    
    CreateRenderTexture(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);

    ElemDataSpan shaderData = SampleReadFile(!applicationPayload->PreferVulkan ? "Fractal.shader": "Fractal_vulkan.shader");
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, shaderData);

    applicationPayload->ComputePipeline = ElemCompileComputePipelineState(applicationPayload->GraphicsDevice, &(ElemComputePipelineStateParameters) {
        .DebugName = "Compute PSO",
        .ShaderLibrary = shaderLibrary,
        .ComputeShaderFunction = "Fractal",
    });

    ElemFreeShaderLibrary(shaderLibrary);

    shaderData = SampleReadFile(!applicationPayload->PreferVulkan ? "Tonemap.shader": "Tonemap_vulkan.shader");
    shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, shaderData);

    applicationPayload->GraphicsPipeline = ElemCompileGraphicsPipelineState(applicationPayload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "Tonemap PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .TextureFormats = { .Items = (ElemTextureFormat[]) { swapChainInfo.Format }, .Length = 1 }
    });

    ElemFreeShaderLibrary(shaderLibrary);

    applicationPayload->ShaderParameters.Transform = CreateIdentityMatrix();
    applicationPayload->InputActions.ShowCursor = true;
    applicationPayload->GameState.Zoom = 1.0f;
    applicationPayload->GameState.RotationDelta = 0.0f;

    RegisterInputBindings(applicationPayload);
    
    SampleStartFrameMeasurement();
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemFreePipelineState(applicationPayload->ComputePipeline);
    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreeTexture(applicationPayload->RenderTexture);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
    ElemFreeGraphicsHeap(applicationPayload->GraphicsHeap);
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
    gameState->TranslationDelta = V2Zero; 

    float zoomDelta = 0.0f;
    float rotationDeltaZ = 0.0f;

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
                zoomDelta = (distance - gameState->PreviousTouchDistance) * ZOOM_MULTITOUCH_SPEED * deltaTimeInSeconds;
            }

            if (gameState->PreviousTouchAngle != 0.0f)
            {
                rotationDeltaZ = -NormalizeAngle(angle - gameState->PreviousTouchAngle) * ROTATION_MULTITOUCH_SPEED * deltaTimeInSeconds;
            }

            gameState->PreviousTouchDistance = distance;
            gameState->PreviousTouchAngle = angle;
        }
        else 
        {
            ResetTouchParameters(gameState);

            gameState->TranslationDelta.X = (inputActions->TouchTranslateRight - inputActions->TouchTranslateLeft) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
            gameState->TranslationDelta.Y = (inputActions->TouchTranslateDown - inputActions->TouchTranslateUp) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
        }
    }
    else if (inputActions->TouchRotateSide)
    {
        ResetTouchParameters(gameState);
        rotationDeltaZ = (inputActions->TouchTranslateRight - inputActions->TouchTranslateLeft) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
    }
    else if (inputActions->TouchReleased && !inputActions->Touch2)
    {
        ResetTouchParameters(gameState);

        gameState->RotationTouch.X = (inputActions->TouchTranslateUp - inputActions->TouchTranslateDown) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
        gameState->RotationTouch.Y = (inputActions->TouchTranslateLeft - inputActions->TouchTranslateRight) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
    }
    else
    {
        Vector2 direction = NormalizeV2((Vector2) 
        { 
            .X = (inputActions->TranslateRight - inputActions->TranslateLeft),
            .Y = (inputActions->TranslateDown - inputActions->TranslateUp),
        });

        if (MagnitudeSquaredV2(direction))
        {
            Vector2 acceleration = AddV2(MulScalarV2(direction, ROTATION_ACCELERATION), MulScalarV2(InverseV2(gameState->CurrentTranslationSpeed), ROTATION_FRICTION));

            ResetTouchParameters(gameState);

            gameState->TranslationDelta = AddV2(MulScalarV2(acceleration, 0.5f * pow2f(deltaTimeInSeconds)), MulScalarV2(gameState->CurrentTranslationSpeed, deltaTimeInSeconds));
            gameState->CurrentTranslationSpeed = AddV2(MulScalarV2(acceleration, deltaTimeInSeconds), gameState->CurrentTranslationSpeed);
        }

        rotationDeltaZ = (inputActions->RotateSideLeft - inputActions->RotateSideRight) * 1 * deltaTimeInSeconds;
    }

    zoomDelta = -(inputActions->ZoomIn - inputActions->ZoomOut) * ZOOM_SPEED * deltaTimeInSeconds; 

    if (zoomDelta != 0)
    {
        if (zoomDelta > 0) 
        {
            gameState->Zoom *= powf(ZOOM_FACTOR, zoomDelta);
        }
        else
        {
            gameState->Zoom /= powf(ZOOM_FACTOR, -zoomDelta);
        }
    }

    gameState->TranslationDelta = MulScalarV2(gameState->TranslationDelta, gameState->Zoom);
    gameState->RotationDelta = rotationDeltaZ;
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    if (updateParameters->SizeChanged)
    {
        CreateRenderTexture(applicationPayload, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);
    }

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

    Matrix3x3 transformMatrix = MulMatrix3x3(CreateTranslationMatrix(gameState->TranslationDelta.X, gameState->TranslationDelta.Y), CreateRotationMatrix(gameState->RotationDelta));

    applicationPayload->ShaderParameters.Transform = MulMatrix3x3(applicationPayload->ShaderParameters.Transform, transformMatrix);
    applicationPayload->ShaderParameters.Zoom = gameState->Zoom;
    applicationPayload->ShaderParameters.TriangeColor = inputActions->TriangleColor;

    // TODO: Do an example with another compute queue to demonstrate fences?

    ElemCommandList commandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL); 

    ElemBindGraphicsHeap(commandList, applicationPayload->GraphicsHeap);
    ElemBindPipelineState(commandList, applicationPayload->ComputePipeline);
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });

    ElemDispatchCompute(commandList, (updateParameters->SwapChainInfo.Width + 15) / 16, (updateParameters->SwapChainInfo.Height + 15) / 16, 1);

    // TODO: Barrier here?

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
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->RenderTextureReadDescriptor, .Length = sizeof(uint32_t) });
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

