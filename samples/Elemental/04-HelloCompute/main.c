#include <math.h>
#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"
#include "SampleInputs.h"

#define ROTATION_TOUCH_SPEED 0.5f
#define ROTATION_TOUCH_MAX_DELTA 0.3f
#define ROTATION_MULTITOUCH_SPEED 100.0f
#define ROTATION_ACCELERATION 50.0f
#define ROTATION_FRICTION 40.0f
#define ZOOM_MULTITOUCH_SPEED 100.0f
#define ZOOM_SPEED 0.5f
#define ZOOM_FACTOR 10.0f

typedef struct
{
    uint32_t RenderTextureIndex;
    float Zoom;
    uint32_t Padding[2];
    SampleMatrix3x3 Transform;
} ShaderParameters;

typedef struct
{
    SampleVector2 TranslationDelta;
    float RotationDelta;
    SampleVector2 RotationTouch;
    SampleVector2 CurrentTranslationSpeed;
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
    ElemGraphicsResource RenderTexture;
    ElemGraphicsResourceDescriptor RenderTextureWriteDescriptor;
    ElemGraphicsResourceDescriptor RenderTextureReadDescriptor;
    ElemFence LastExecutionFence;
    ElemSwapChain SwapChain;
    ElemPipelineState GraphicsPipeline;
    ElemPipelineState ComputePipeline;
    ShaderParameters ShaderParameters;
    SampleStandardInputActions InputActions;
    SampleInputActionBindingSpan InputActionBindings;
    GameState GameState;
} ApplicationPayload;
    
void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

// NOTE: Here we can reuse memory and delete resources directly because we use the default latency for the swapchain that
// is set to 1. So it means the GPU is not using the resources while this code is executed.
// If the latency is configured to 2. You need to be carreful to not overlap the memory in the heap and use a fence for the 
// delete functions.
void CreateRenderTexture(ApplicationPayload* applicationPayload, uint32_t width, uint32_t height)
{
    if (applicationPayload->RenderTexture != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsResourceDescriptor(applicationPayload->RenderTextureReadDescriptor, NULL);
        ElemFreeGraphicsResourceDescriptor(applicationPayload->RenderTextureWriteDescriptor, NULL);

        ElemFreeGraphicsResource(applicationPayload->RenderTexture, NULL);
    }

    printf("Creating render texture...\n");

    ElemGraphicsResourceInfo resourceInfo = ElemCreateTexture2DResourceInfo(applicationPayload->GraphicsDevice, width, height, 1, ElemGraphicsFormat_R16G16B16A16_FLOAT, ElemGraphicsResourceUsage_Write,
                                                                            &(ElemGraphicsResourceInfoOptions) { 
                                                                                .DebugName = "Render Texture" 
                                                                            });

    applicationPayload->RenderTexture = ElemCreateGraphicsResource(applicationPayload->GraphicsHeap, 0, &resourceInfo);

    applicationPayload->RenderTextureReadDescriptor = ElemCreateGraphicsResourceDescriptor(applicationPayload->RenderTexture, ElemGraphicsResourceDescriptorUsage_Read, NULL);
    applicationPayload->RenderTextureWriteDescriptor = ElemCreateGraphicsResourceDescriptor(applicationPayload->RenderTexture, ElemGraphicsResourceDescriptorUsage_Write, NULL);
}

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->Window = ElemCreateWindow(NULL);

    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = true, .EnableGpuValidation = false, .EnableDebugBarrierInfo = false, .PreferVulkan = applicationPayload->PreferVulkan });
    applicationPayload->GraphicsDevice = ElemCreateGraphicsDevice(NULL);

    applicationPayload->CommandQueue= ElemCreateCommandQueue(applicationPayload->GraphicsDevice, ElemCommandQueueType_Graphics, NULL);
    applicationPayload->SwapChain= ElemCreateSwapChain(applicationPayload->CommandQueue, applicationPayload->Window, UpdateSwapChain, &(ElemSwapChainOptions) { .FrameLatency = 1, .UpdatePayload = payload });
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
        .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {{ .Format = swapChainInfo.Format }}, .Length = 1 },
    });

    ElemFreeShaderLibrary(shaderLibrary);

    applicationPayload->ShaderParameters.Transform = SampleCreateIdentityMatrix();
    applicationPayload->InputActions.ShowCursor = true;
    applicationPayload->GameState.Zoom = 1.0f;
    applicationPayload->GameState.RotationDelta = 0.0f;

    SampleRegisterStandardInputBindings(&applicationPayload->InputActionBindings, &applicationPayload->InputActions);
    
    SampleStartFrameMeasurement();
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemWaitForFenceOnCpu(applicationPayload->LastExecutionFence);

    ElemFreePipelineState(applicationPayload->ComputePipeline);
    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreeGraphicsResourceDescriptor(applicationPayload->RenderTextureReadDescriptor, NULL);
    ElemFreeGraphicsResourceDescriptor(applicationPayload->RenderTextureWriteDescriptor, NULL);
    ElemFreeGraphicsResource(applicationPayload->RenderTexture, NULL);
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

void UpdateGameState(GameState* gameState, SampleStandardInputActions* inputActions, float deltaTimeInSeconds)
{
    gameState->TranslationDelta = V2Zero; 

    float zoomDelta = 0.0f;
    float rotationDeltaZ = 0.0f;

    if (inputActions->Touch)
    {
        if (inputActions->Touch2)
        {
            SampleVector2 touchPosition = (SampleVector2) { inputActions->TouchPositionX, inputActions->TouchPositionY };
            SampleVector2 touchPosition2 = (SampleVector2) { inputActions->Touch2PositionX, inputActions->Touch2PositionY };

            SampleVector2 diffVector = SampleSubstractV2(touchPosition, touchPosition2);
            float distance = SampleMagnitudeV2(diffVector);
            float angle = atan2(diffVector.X, diffVector.Y);

            if (gameState->PreviousTouchDistance != 0.0f)
            {
                zoomDelta = -(distance - gameState->PreviousTouchDistance) * ZOOM_MULTITOUCH_SPEED * deltaTimeInSeconds;
            }

            if (gameState->PreviousTouchAngle != 0.0f)
            {
                rotationDeltaZ = -SampleNormalizeAngle(angle - gameState->PreviousTouchAngle) * ROTATION_MULTITOUCH_SPEED * deltaTimeInSeconds;
            }

            gameState->PreviousTouchDistance = distance;
            gameState->PreviousTouchAngle = angle;
        }
        else 
        {
            ResetTouchParameters(gameState);

            gameState->TranslationDelta.X = (inputActions->TouchTranslateLeft - inputActions->TouchTranslateRight) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
            gameState->TranslationDelta.Y = (inputActions->TouchTranslateUp - inputActions->TouchTranslateDown) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
        }
    }
    else if (inputActions->TouchRotateSide)
    {
        ResetTouchParameters(gameState);
        rotationDeltaZ = (inputActions->TouchTranslateLeft - inputActions->TouchTranslateRight) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
    }
    else if (inputActions->TouchReleased && !inputActions->Touch2)
    {
        ResetTouchParameters(gameState);

        gameState->RotationTouch.X = (inputActions->TouchTranslateUp - inputActions->TouchTranslateDown) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
        gameState->RotationTouch.Y = (inputActions->TouchTranslateLeft - inputActions->TouchTranslateRight) * ROTATION_TOUCH_SPEED * deltaTimeInSeconds;
    }
    else
    {
        SampleVector2 direction = SampleNormalizeV2((SampleVector2) 
        { 
            .X = (inputActions->TranslateRight - inputActions->TranslateLeft),
            .Y = (inputActions->TranslateDown - inputActions->TranslateUp),
        });

        if (SampleMagnitudeSquaredV2(direction))
        {
            SampleVector2 acceleration = SampleAddV2(SampleMulScalarV2(direction, ROTATION_ACCELERATION), SampleMulScalarV2(SampleInverseV2(gameState->CurrentTranslationSpeed), ROTATION_FRICTION));

            ResetTouchParameters(gameState);

            gameState->TranslationDelta = SampleAddV2(SampleMulScalarV2(acceleration, 0.5f * SamplePow2f(deltaTimeInSeconds)), SampleMulScalarV2(gameState->CurrentTranslationSpeed, deltaTimeInSeconds));
            gameState->CurrentTranslationSpeed = SampleAddV2(SampleMulScalarV2(acceleration, deltaTimeInSeconds), gameState->CurrentTranslationSpeed);
        }

        rotationDeltaZ = (inputActions->RotateSideLeft - inputActions->RotateSideRight) * 1 * deltaTimeInSeconds;
        zoomDelta = -(inputActions->ZoomIn - inputActions->ZoomOut) * ZOOM_SPEED * deltaTimeInSeconds; 
    }

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

    gameState->TranslationDelta = SampleMulScalarV2(gameState->TranslationDelta, gameState->Zoom);
    gameState->RotationDelta = rotationDeltaZ;
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    if (updateParameters->SizeChanged)
    {
        CreateRenderTexture(applicationPayload, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);
    }

    applicationPayload->ShaderParameters.RenderTextureIndex = applicationPayload->RenderTextureWriteDescriptor;

    ElemInputStream inputStream = ElemGetInputStream();
    SampleStandardInputActions* inputActions = &applicationPayload->InputActions;
    SampleUpdateInputActions(&applicationPayload->InputActionBindings, inputStream);

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

    SampleMatrix3x3 transformMatrix = SampleMulMatrix3x3(SampleCreateTranslationMatrix(gameState->TranslationDelta.X, gameState->TranslationDelta.Y), SampleCreateRotationMatrix(gameState->RotationDelta));

    applicationPayload->ShaderParameters.Transform = SampleMulMatrix3x3(applicationPayload->ShaderParameters.Transform, transformMatrix);
    applicationPayload->ShaderParameters.Zoom = gameState->Zoom;

    // TODO: Do an example with another compute queue to demonstrate fences?

    ElemCommandList commandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL); 

    ElemBindPipelineState(commandList, applicationPayload->ComputePipeline);
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });

    // BUG: On iOS when the framerate is slower (we miss the vsync), the barrier here is not kicked in and the gpu overlapp the render encoders
    // For this case, it seems it is caused by calling the update handler for vsync regardless of if the frame was presented

    ElemGraphicsResourceBarrier(commandList, applicationPayload->RenderTextureWriteDescriptor, NULL);

    uint32_t threadSize = 16;
    ElemDispatchCompute(commandList, (updateParameters->SwapChainInfo.Width + (threadSize - 1)) / threadSize, (updateParameters->SwapChainInfo.Height + (threadSize - 1)) / threadSize, 1);

    ElemGraphicsResourceBarrier(commandList, applicationPayload->RenderTextureReadDescriptor, NULL);

    ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters) {
        .RenderTargets = 
        {
            .Items = (ElemRenderPassRenderTarget[]) { 
            {
                .RenderTarget = updateParameters->BackBufferRenderTarget,
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
    applicationPayload->LastExecutionFence = ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, NULL);

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
        .ApplicationName = "Hello Compute",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });
}
