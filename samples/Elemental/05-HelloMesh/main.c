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
    uint32_t VertexBuffer;
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

typedef struct
{
    ElemGraphicsResource VertexBuffer;
    ElemGraphicsResourceDescriptor VertexBufferReadDescriptor;
} MeshData;

// TODO: Group common variables into separate structs
typedef struct
{
    bool PreferVulkan;
    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    ElemGraphicsHeap GraphicsHeap;
    uint32_t CurrentHeapOffset;
    ElemFence LastExecutionFence;
    ElemSwapChain SwapChain;
    ElemPipelineState GraphicsPipeline;
    ShaderParameters ShaderParameters;
    SampleStandardInputActions InputActions;
    SampleInputActionBindingSpan InputActionBindings;
    GameState GameState;
    MeshData TestMeshData;
} ApplicationPayload;
    
void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

void LoadMesh(MeshData* meshData, const char* path, ApplicationPayload* applicationPayload)
{
    // TODO: Alignment should be used with the offset before adding the size of the resource!
    ElemGraphicsResourceInfo bufferDescription = ElemCreateGraphicsBufferResourceInfo(applicationPayload->GraphicsDevice, 10 * sizeof(SampleVector3), ElemGraphicsResourceUsage_Read, NULL);

    applicationPayload->CurrentHeapOffset = SampleAlignValue(applicationPayload->CurrentHeapOffset, bufferDescription.Alignment);
    meshData->VertexBuffer = ElemCreateGraphicsResource(applicationPayload->GraphicsHeap, applicationPayload->CurrentHeapOffset, &bufferDescription);
    applicationPayload->CurrentHeapOffset += bufferDescription.SizeInBytes;

    float vertices[] =
    {
        -1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        -1.0f, 0.0f, 0.0f
    };

    ElemDataSpan vertexBufferPointer = ElemGetGraphicsResourceDataSpan(applicationPayload->TestMeshData.VertexBuffer);
    memcpy(vertexBufferPointer.Items, vertices, sizeof(vertices));

    meshData->VertexBufferReadDescriptor = ElemCreateGraphicsResourceDescriptor(applicationPayload->TestMeshData.VertexBuffer, ElemGraphicsResourceDescriptorUsage_Read, NULL);
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

    // TODO: For now we need to put the heap as GpuUpload but it should be Gpu when we use IOQueues
    applicationPayload->GraphicsHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_GpuUpload });

    LoadMesh(&applicationPayload->TestMeshData, "Pouet.mesh", applicationPayload);
    applicationPayload->ShaderParameters.VertexBuffer = applicationPayload->TestMeshData.VertexBufferReadDescriptor;

    ElemDataSpan shaderData = SampleReadFile(!applicationPayload->PreferVulkan ? "RenderMesh.shader": "RenderMesh_vulkan.shader");
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, shaderData);

    applicationPayload->GraphicsPipeline = ElemCompileGraphicsPipelineState(applicationPayload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "RenderMesh PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .TextureFormats = { .Items = (ElemGraphicsFormat[]) { swapChainInfo.Format }, .Length = 1 }
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

    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
 
    // Free Mesh
    ElemFreeGraphicsResourceDescriptor(applicationPayload->TestMeshData.VertexBufferReadDescriptor, NULL);
    ElemFreeGraphicsResource(applicationPayload->TestMeshData.VertexBuffer, NULL);

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

    SampleStandardInputActions* inputActions = &applicationPayload->InputActions;
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

    SampleMatrix3x3 transformMatrix = SampleMulMatrix3x3(SampleCreateTranslationMatrix(gameState->TranslationDelta.X, gameState->TranslationDelta.Y), SampleCreateRotationMatrix(gameState->RotationDelta));

    applicationPayload->ShaderParameters.Transform = SampleMulMatrix3x3(applicationPayload->ShaderParameters.Transform, transformMatrix);

    ElemCommandList commandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL); 

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
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });
    ElemDispatchMesh(commandList, 1, 1, 1);

    ElemEndRenderPass(commandList);

    ElemCommitCommandList(commandList);
    applicationPayload->LastExecutionFence = ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, NULL);

    ElemPresentSwapChain(applicationPayload->SwapChain);
    SampleFrameMeasurement frameMeasurement = SampleEndFrameMeasurement();

    if (frameMeasurement.HasNewData)
    {
        SampleSetWindowTitle(applicationPayload->Window, "HelloMesh", applicationPayload->GraphicsDevice, frameMeasurement.FrameTimeInSeconds, frameMeasurement.Fps);
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
        .ApplicationName = "Hello Mesh",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });
}
