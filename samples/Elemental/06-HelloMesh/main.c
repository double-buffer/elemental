#include <math.h>
#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"
#include "SampleInputs.h"
#include "SampleMesh.h"

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
    uint32_t VertexBuffer;
    uint32_t MeshletBuffer;
    uint32_t MeshletVertexIndexBuffer;
    uint32_t MeshletTriangleIndexBuffer;
    SampleVector4 RotationQuaternion;
    float Zoom;
    float AspectRatio;
    uint32_t TriangeColor;
    uint32_t MeshletCount;
} ShaderParameters;

// TODO: Extract from the gamestate the inputState
typedef struct
{
    SampleVector3 RotationDelta;
    SampleVector2 RotationTouch;
    SampleVector3 CurrentRotationSpeed;
    float PreviousTouchDistance;
    float PreviousTouchAngle;
    float Zoom;
} GameState;

typedef struct
{
    uint32_t MeshletCount;
    ElemGraphicsResource VertexBuffer;
    ElemGraphicsResourceDescriptor VertexBufferReadDescriptor;
    ElemGraphicsResource MeshletBuffer;
    ElemGraphicsResourceDescriptor MeshletBufferReadDescriptor;
    ElemGraphicsResource MeshletVertexIndexBuffer;
    ElemGraphicsResourceDescriptor MeshletVertexIndexBufferReadDescriptor;
    ElemGraphicsResource MeshletTriangleIndexBuffer;
    ElemGraphicsResourceDescriptor MeshletTriangleIndexBufferReadDescriptor;
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
    ElemGraphicsHeap DepthBufferHeap;
    ElemGraphicsResource DepthBuffer;
    ElemPipelineState GraphicsPipeline;
    ShaderParameters ShaderParameters;
    SampleModelViewerInputActions InputActions;
    SampleInputActionBindingSpan InputActionBindings;
    GameState GameState;
    MeshData TestMeshData;
} ApplicationPayload;
    
void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

void CreateDepthBuffer(ApplicationPayload* applicationPayload, uint32_t width, uint32_t height)
{
    if (applicationPayload->DepthBuffer != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    }

    printf("Creating DepthBuffer...\n");

    ElemGraphicsResourceInfo resourceInfo = ElemCreateTexture2DResourceInfo(applicationPayload->GraphicsDevice, width, height, 1, ElemGraphicsFormat_D32_FLOAT, ElemGraphicsResourceUsage_DepthStencil,
                                                                            &(ElemGraphicsResourceInfoOptions) { 
                                                                                .DebugName = "DepthBuffer" 
                                                                            });

    applicationPayload->DepthBuffer = ElemCreateGraphicsResource(applicationPayload->DepthBufferHeap, 0, &resourceInfo);
}

// TODO: To remove when IOQueues
void CreateAndUploadDataTemp(ElemGraphicsResource* buffer, ElemGraphicsResourceDescriptor* readDescriptor, ApplicationPayload* applicationPayload, void* dataPointer, uint32_t sizeInBytes)
{
    // TODO: Alignment should be used with the offset before adding the size of the resource!
    ElemGraphicsResourceInfo bufferDescription = ElemCreateGraphicsBufferResourceInfo(applicationPayload->GraphicsDevice, sizeInBytes, ElemGraphicsResourceUsage_Read, NULL);

    applicationPayload->CurrentHeapOffset = SampleAlignValue(applicationPayload->CurrentHeapOffset, bufferDescription.Alignment);
    *buffer = ElemCreateGraphicsResource(applicationPayload->GraphicsHeap, applicationPayload->CurrentHeapOffset, &bufferDescription);
    applicationPayload->CurrentHeapOffset += bufferDescription.SizeInBytes;

    ElemDataSpan vertexBufferPointer = ElemGetGraphicsResourceDataSpan(*buffer);
    memcpy(vertexBufferPointer.Items, dataPointer, sizeInBytes);

    *readDescriptor = ElemCreateGraphicsResourceDescriptor(*buffer, ElemGraphicsResourceDescriptorUsage_Read, NULL);
}

void LoadMesh(MeshData* meshData, const char* path, ApplicationPayload* applicationPayload)
{
    // TODO: When IOQueues are implemented, we only need to read the header, not the whole file!
    // Add a parameter to specify the length we want to read and the offset
    ElemDataSpan meshFileData = SampleReadFile(path);

    uint8_t* fileDataPointer = meshFileData.Items;
    SampleMeshHeader* meshHeader = (SampleMeshHeader*)fileDataPointer;

    if (meshHeader->FileId[0] == 'M' && meshHeader->FileId[1] == 'E' && meshHeader->FileId[2] == 'S' && meshHeader->FileId[3] == 'H')
    {
        printf("OK Meshlet Count: %d\n", meshHeader->MeshletCount);
    }

    meshData->MeshletCount = meshHeader->MeshletCount;

    CreateAndUploadDataTemp(&meshData->VertexBuffer, &meshData->VertexBufferReadDescriptor, applicationPayload, fileDataPointer + meshHeader->VertexBufferOffset, meshHeader->VertexBufferSizeInBytes);
    CreateAndUploadDataTemp(&meshData->MeshletBuffer, &meshData->MeshletBufferReadDescriptor, applicationPayload, fileDataPointer + meshHeader->MeshletBufferOffset, meshHeader->MeshletBufferSizeInBytes);
    CreateAndUploadDataTemp(&meshData->MeshletVertexIndexBuffer, &meshData->MeshletVertexIndexBufferReadDescriptor, applicationPayload, fileDataPointer + meshHeader->MeshletVertexIndexBufferOffset, meshHeader->MeshletVertexIndexBufferSizeInBytes);
    CreateAndUploadDataTemp(&meshData->MeshletTriangleIndexBuffer, &meshData->MeshletTriangleIndexBufferReadDescriptor, applicationPayload, fileDataPointer + meshHeader->MeshletTriangleIndexBufferOffset, meshHeader->MeshletTriangleIndexBufferSizeInBytes);

    applicationPayload->ShaderParameters.VertexBuffer = meshData->VertexBufferReadDescriptor;
    applicationPayload->ShaderParameters.MeshletBuffer = meshData->MeshletBufferReadDescriptor;
    applicationPayload->ShaderParameters.MeshletVertexIndexBuffer = meshData->MeshletVertexIndexBufferReadDescriptor;
    applicationPayload->ShaderParameters.MeshletTriangleIndexBuffer = meshData->MeshletTriangleIndexBufferReadDescriptor;
    applicationPayload->ShaderParameters.MeshletCount = meshData->MeshletCount;
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

    // TODO: For now we create a separate heap to avoid memory management
    applicationPayload->DepthBufferHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_Gpu });

    // TODO: For now we need to put the heap as GpuUpload but it should be Gpu when we use IOQueues
    applicationPayload->GraphicsHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_GpuUpload });

    CreateDepthBuffer(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);
    LoadMesh(&applicationPayload->TestMeshData, "kitten.mesh", applicationPayload);

    ElemDataSpan shaderData = SampleReadFile(!applicationPayload->PreferVulkan ? "RenderMesh.shader": "RenderMesh_vulkan.shader");
    ElemShaderLibrary shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, shaderData);

    applicationPayload->GraphicsPipeline = ElemCompileGraphicsPipelineState(applicationPayload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "RenderMesh PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {{ .Format = swapChainInfo.Format }}, .Length = 1 },
        .DepthStencil =
        {
            .Format = ElemGraphicsFormat_D32_FLOAT,
            .DepthCompareFunction = ElemGraphicsCompareFunction_Greater
        }
    });

    ElemFreeShaderLibrary(shaderLibrary);

    applicationPayload->ShaderParameters.RotationQuaternion = (SampleVector4){ .X = 0, .Y = 0, .Z = 0, .W = 1 };
    applicationPayload->InputActions.ShowCursor = true;

    SampleRegisterModelViewerInputBindings(&applicationPayload->InputActionBindings, &applicationPayload->InputActions);
    
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
    ElemFreeGraphicsResourceDescriptor(applicationPayload->TestMeshData.MeshletBufferReadDescriptor, NULL);
    ElemFreeGraphicsResource(applicationPayload->TestMeshData.MeshletBuffer, NULL);
    ElemFreeGraphicsResourceDescriptor(applicationPayload->TestMeshData.MeshletVertexIndexBufferReadDescriptor, NULL);
    ElemFreeGraphicsResource(applicationPayload->TestMeshData.MeshletVertexIndexBuffer, NULL);
    ElemFreeGraphicsResourceDescriptor(applicationPayload->TestMeshData.MeshletTriangleIndexBufferReadDescriptor, NULL);
    ElemFreeGraphicsResource(applicationPayload->TestMeshData.MeshletTriangleIndexBuffer, NULL);

    ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    ElemFreeGraphicsHeap(applicationPayload->DepthBufferHeap);

    ElemFreeGraphicsHeap(applicationPayload->GraphicsHeap);
    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);
}

void ResetTouchParameters(GameState* gameState) 
{
    gameState->RotationTouch = V2Zero;
    gameState->PreviousTouchDistance = 0.0f;
    gameState->PreviousTouchAngle = 0.0f;   
}

// TODO: Extract model view gamestate update
void UpdateGameState(GameState* gameState, SampleModelViewerInputActions* inputActions, float deltaTimeInSeconds)
{
    gameState->RotationDelta = V3Zero; 

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
                gameState->Zoom += (distance - gameState->PreviousTouchDistance) * ZOOM_MULTITOUCH_SPEED * deltaTimeInSeconds;
            }

            if (gameState->PreviousTouchAngle != 0.0f)
            {
                gameState->RotationDelta.Z = -SampleNormalizeAngle(angle - gameState->PreviousTouchAngle) * ROTATION_MULTITOUCH_SPEED * deltaTimeInSeconds;
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
        SampleVector3 direction = SampleNormalizeV3((SampleVector3) 
        { 
            .X = (inputActions->RotateUp - inputActions->RotateDown),
            .Y = (inputActions->RotateLeft - inputActions->RotateRight),
            .Z = (inputActions->RotateSideLeft - inputActions->RotateSideRight)
        });

        if (SampleMagnitudeSquaredV3(direction))
        {
            SampleVector3 acceleration = SampleAddV3(SampleMulScalarV3(direction, ROTATION_ACCELERATION), SampleMulScalarV3(SampleInverseV3(gameState->CurrentRotationSpeed), ROTATION_FRICTION));

            ResetTouchParameters(gameState);
            gameState->RotationDelta = SampleAddV3(SampleMulScalarV3(acceleration, 0.5f * SamplePow2f(deltaTimeInSeconds)), SampleMulScalarV3(gameState->CurrentRotationSpeed, deltaTimeInSeconds));
            gameState->CurrentRotationSpeed = SampleAddV3(SampleMulScalarV3(acceleration, deltaTimeInSeconds), gameState->CurrentRotationSpeed);
        }
    }

    if (SampleMagnitudeSquaredV2(gameState->RotationTouch) > 0)
    {
        if (SampleMagnitudeV2(gameState->RotationTouch) > ROTATION_TOUCH_MAX_DELTA)
        {
            gameState->RotationTouch = SampleMulScalarV2(SampleNormalizeV2(gameState->RotationTouch), ROTATION_TOUCH_MAX_DELTA);
        }

        gameState->RotationDelta = SampleAddV3(gameState->RotationDelta, (SampleVector3){ gameState->RotationTouch.X, gameState->RotationTouch.Y, 0.0f });

        SampleVector2 inverse = SampleMulScalarV2(SampleNormalizeV2(SampleInverseV2(gameState->RotationTouch)), ROTATION_TOUCH_DECREASE_SPEED);
        gameState->RotationTouch = SampleAddV2(gameState->RotationTouch, inverse);

        if (SampleMagnitudeV2(gameState->RotationTouch) < 0.001f)
        {
            ResetTouchParameters(gameState);
        }
    }

    gameState->Zoom += (inputActions->ZoomIn - inputActions->ZoomOut) * ZOOM_SPEED * deltaTimeInSeconds; 
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    
    if (updateParameters->SizeChanged)
    {
        CreateDepthBuffer(applicationPayload, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);
    }

    SampleModelViewerInputActions* inputActions = &applicationPayload->InputActions;
    // TODO: PAss the input stream as a parameter
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

    if (SampleMagnitudeSquaredV3(gameState->RotationDelta))
    {
        SampleVector4 rotationQuaternion = SampleMulQuat(SampleCreateQuaternion((SampleVector3){ 1, 0, 0 }, gameState->RotationDelta.X), 
                                                         SampleMulQuat(SampleCreateQuaternion((SampleVector3){ 0, 0, 1 }, gameState->RotationDelta.Z),
                                                                       SampleCreateQuaternion((SampleVector3){ 0, 1, 0 }, gameState->RotationDelta.Y)));

        applicationPayload->ShaderParameters.RotationQuaternion = SampleMulQuat(rotationQuaternion, applicationPayload->ShaderParameters.RotationQuaternion);
    }

    applicationPayload->ShaderParameters.AspectRatio = updateParameters->SwapChainInfo.AspectRatio;
    float maxZoom = (applicationPayload->ShaderParameters.AspectRatio >= 0.75 ? 1.5f : 3.5f);
    applicationPayload->ShaderParameters.Zoom = fminf(maxZoom, gameState->Zoom);
    applicationPayload->ShaderParameters.TriangeColor = inputActions->TriangleColor;

    ElemCommandList commandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL); 

    ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters) {
        .RenderTargets = 
        {
            .Items = (ElemRenderPassRenderTarget[]) { 
            {
                .RenderTarget = updateParameters->BackBufferRenderTarget,
                .ClearColor = { 0.0f, 0.01f, 0.02f, 1.0f },
            }},
            .Length = 1
        },
        .DepthStencil =
        {
            .DepthStencil = applicationPayload->DepthBuffer
        }
    });

    ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline); 
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });
    ElemDispatchMesh(commandList, applicationPayload->TestMeshData.MeshletCount, 1, 1);

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
