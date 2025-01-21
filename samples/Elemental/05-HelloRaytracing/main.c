#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"
#include "SampleInputsApplication.h"
#include "SampleInputsCamera.h"
#include "SampleSceneLoader.h"

typedef struct
{
    uint32_t ShaderGlobalParametersBuffer;
    uint32_t PrimitiveId;
    uint32_t MeshInstanceId;
} ShaderParameters;

typedef struct
{
    uint32_t AccelerationStructureIndex;
    uint32_t ShaderGlobalParametersBufferIndex;
    uint32_t MaterialBuffer;
} RaytracingShaderParameters;

typedef struct
{
    SampleMatrix4x4 ViewProjMatrix;
    SampleMatrix4x4 InverseViewMatrix;
    SampleMatrix4x4 InverseProjectionMatrix;
    uint32_t MaterialBufferIndex;
    uint32_t GpuMeshInstanceBufferIndex;
    uint32_t GpuMeshPrimitiveInstanceBufferIndex;
    uint32_t Action;
} ShaderShaderGlobalParameters;

// TODO: Group common variables into separate structs
typedef struct
{
    SampleAppSettings AppSettings;
    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    uint32_t CurrentHeapOffset;
    ElemFence LastExecutionFence;
    ElemSwapChain SwapChain;
    ElemGraphicsHeap DepthBufferHeap;
    ElemGraphicsResource DepthBuffer;
    ElemPipelineState GraphicsPipeline;
    ElemPipelineState RaytracingGraphicsPipeline;
    ShaderParameters ShaderParameters;
    SampleInputsApplication InputsApplication;
    SampleInputsCamera InputsCamera;
    SampleSceneData TestSceneData;
    SampleGpuMemory GpuMemory;
    SampleGpuMemory GpuMemoryUpload;

    ShaderShaderGlobalParameters ShaderGlobalParameters;
    SampleGpuBuffer ShaderGlobalParametersBuffer;
    
    SampleGpuTexture RenderTargetTexture;
} ApplicationPayload;
    
typedef struct
{
    SampleInputsCameraState CameraState; 
} SavedState;

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

void CreateRenderTarget(ApplicationPayload* applicationPayload, uint32_t width, uint32_t height)
{
    if (applicationPayload->RenderTargetTexture.Texture != ELEM_HANDLE_NULL)
    {
        SampleFreeGpuTexture(&applicationPayload->RenderTargetTexture);
    }

    printf("Creating render texture...\n");

    applicationPayload->RenderTargetTexture = SampleCreateGpuRenderTarget(&applicationPayload->GpuMemoryUpload, width, height, ElemGraphicsFormat_R32G32B32A32_FLOAT, "RenderTarget");
}

void CreateRaytracingAccelerationStructures(ApplicationPayload* applicationPayload)
{
    // TODO: For the moment we create a GPU buffer for each accel struct
    // TODO: Refactor the code to create only one buffer for the whole app and multiple accel struct

    SampleSceneData* sceneData = &applicationPayload->TestSceneData;

    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleMeshData* meshData = &sceneData->Meshes[i];
        
        for (uint32_t j = 0; j < meshData->MeshHeader.MeshPrimitiveCount; j++)
        {
            SampleMeshPrimitiveData* meshPrimitiveData = &meshData->MeshPrimitives[j];

            // TODO: Change that
            ElemRaytracingBlasParameters blasParameters =
            {
                .BuildFlags = ElemRaytracingBuildFlags_PreferFastTrace,
                .VertexFormat = ElemGraphicsFormat_R32G32B32_FLOAT,
                .VertexBuffer = meshData->MeshBuffer.Buffer,
                .VertexBufferOffset = meshPrimitiveData->PrimitiveHeader.VertexBufferOffset,
                .VertexCount = meshPrimitiveData->PrimitiveHeader.VertexCount,
                .VertexSizeInBytes = meshData->MeshHeader.VertexSizeInBytes,
                .IndexFormat = ElemGraphicsFormat_R32_UINT,
                .IndexBuffer = meshData->MeshBuffer.Buffer,
                .IndexBufferOffset = meshPrimitiveData->PrimitiveHeader.IndexBufferOffset,
                .IndexCount = meshPrimitiveData->PrimitiveHeader.IndexCount
            };

            ElemRaytracingAllocationInfo allocationInfos = ElemGetRaytracingBlasAllocationInfo(applicationPayload->GraphicsDevice, &blasParameters);

            meshPrimitiveData->RaytracingStorageBuffer = SampleCreateGpuRaytracingBuffer(&applicationPayload->GpuMemory, allocationInfos.SizeInBytes, "AccelStorage");
            meshPrimitiveData->RaytracingScratchBuffer = SampleCreateGpuBuffer(&applicationPayload->GpuMemoryUpload, allocationInfos.ScratchSizeInBytes, "ScratchStorage");
            meshPrimitiveData->RaytracingAccelerationStructure = ElemCreateRaytracingAccelerationStructureResource(applicationPayload->GraphicsDevice, meshPrimitiveData->RaytracingStorageBuffer.Buffer, NULL);
        }
    }
        
    ElemGraphicsResourceAllocationInfo tlasInstanceAllocationInfo = ElemGetRaytracingTlasInstanceAllocationInfo(applicationPayload->GraphicsDevice, applicationPayload->TestSceneData.NodeCount);
    sceneData->TlasInstanceBuffer = SampleCreateGpuBuffer(&applicationPayload->GpuMemoryUpload, tlasInstanceAllocationInfo.SizeInBytes, "TlasInstanceBuffer");

    uint32_t tlasInstanceCount = 0u;

    for (uint32_t i = 0; i < applicationPayload->TestSceneData.NodeCount; i++)
    {
        SampleSceneNodeHeader* sceneNode = &applicationPayload->TestSceneData.Nodes[i];

        if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
        {
            SampleMeshData* meshData = &applicationPayload->TestSceneData.Meshes[sceneNode->ReferenceIndex];

            for (uint32_t j = 0; j < meshData->MeshHeader.MeshPrimitiveCount; j++)
            {
                tlasInstanceCount++;
            }
        }
    }

    ElemRaytracingTlasParameters tlasParameters =
    {
        .BuildFlags = ElemRaytracingBuildFlags_PreferFastTrace,
        .InstanceCount = tlasInstanceCount,
    };

    ElemRaytracingAllocationInfo allocationInfos = ElemGetRaytracingTlasAllocationInfo(applicationPayload->GraphicsDevice, &tlasParameters);

    sceneData->RaytracingStorageBuffer = SampleCreateGpuRaytracingBuffer(&applicationPayload->GpuMemory, allocationInfos.SizeInBytes, "TLASAccelStorage");
    sceneData->RaytracingScratchBuffer = SampleCreateGpuBuffer(&applicationPayload->GpuMemoryUpload, allocationInfos.ScratchSizeInBytes, "TLASScratchStorage");
    sceneData->RaytracingAccelerationStructure = ElemCreateRaytracingAccelerationStructureResource(applicationPayload->GraphicsDevice, sceneData->RaytracingStorageBuffer.Buffer, NULL);
    sceneData->RaytracingAccelerationStructureReadDescriptor = ElemCreateGraphicsResourceDescriptor(sceneData->RaytracingAccelerationStructure, ElemGraphicsResourceDescriptorUsage_Read, NULL);
}

void BuildRaytracingAccelerationStructures(ElemCommandList commandList, ApplicationPayload* applicationPayload)
{
    // TODO: Reuse the BLAS/TLAS parameters struct?

    SampleSceneData* sceneData = &applicationPayload->TestSceneData;

    for (uint32_t i = 0; i < sceneData->MeshCount; i++)
    {
        SampleMeshData* meshData = &sceneData->Meshes[i];

        for (uint32_t j = 0; j < meshData->MeshHeader.MeshPrimitiveCount; j++)
        {
            SampleMeshPrimitiveData* meshPrimitiveData = &meshData->MeshPrimitives[j];

            // TODO: Here we have to force the before access, change that
            // TODO: Do we really need that one?
            ElemGraphicsResourceBarrierOptions barrierOptions = 
            {
                .BeforeSync = ElemGraphicsResourceBarrierSyncType_Copy,
                .BeforeAccess = ElemGraphicsResourceBarrierAccessType_Copy
            };

            // TODO: Should be write here
            ElemGraphicsResourceBarrier(commandList, meshData->MeshBuffer.ReadDescriptor, &barrierOptions);

            ElemRaytracingBlasParameters blasParameters =
            {
                .BuildFlags = ElemRaytracingBuildFlags_PreferFastTrace,
                .VertexFormat = ElemGraphicsFormat_R32G32B32_FLOAT,
                .VertexBuffer = meshData->MeshBuffer.Buffer,
                .VertexBufferOffset = meshPrimitiveData->PrimitiveHeader.VertexBufferOffset,
                .VertexCount = meshPrimitiveData->PrimitiveHeader.VertexCount,
                .VertexSizeInBytes = meshData->MeshHeader.VertexSizeInBytes,
                .IndexFormat = ElemGraphicsFormat_R32_UINT,
                .IndexBuffer = meshData->MeshBuffer.Buffer,
                .IndexBufferOffset = meshPrimitiveData->PrimitiveHeader.IndexBufferOffset,
                .IndexCount = meshPrimitiveData->PrimitiveHeader.IndexCount
            };

            ElemBuildRaytracingBlas(commandList, meshPrimitiveData->RaytracingAccelerationStructure, meshPrimitiveData->RaytracingScratchBuffer.Buffer, &blasParameters, NULL);

            ElemGraphicsResourceBarrier(commandList, meshData->MeshBuffer.ReadDescriptor, &barrierOptions);
            ElemGraphicsResourceBarrier(commandList, meshPrimitiveData->RaytracingStorageBuffer.ReadDescriptor, NULL);
        }
    }

    // TODO: Move that part in the other function
    ElemRaytracingTlasInstance tlasInstances[1024];
    uint32_t tlasInstanceCount = 0u;

    for (uint32_t i = 0; i < sceneData->NodeCount; i++)
    {
        SampleSceneNodeHeader* sceneNode = &sceneData->Nodes[i];

        if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
        {
            SampleMeshData* meshData = &sceneData->Meshes[sceneNode->ReferenceIndex];

            for (uint32_t j = 0; j < meshData->MeshHeader.MeshPrimitiveCount; j++)
            {
                SampleMeshPrimitiveData* meshPrimitiveData = &meshData->MeshPrimitives[j];

                // TODO: Scale
                ElemMatrix4x3 transformMatrix = SampleCreateTransformMatrix2(sceneNode->Rotation, sceneNode->Translation);

                tlasInstances[tlasInstanceCount++] = (ElemRaytracingTlasInstance)
                {
                    .InstanceId = i,
                    .InstanceFlags = ElemRaytracingTlasInstanceFlags_DisableTriangleCulling,
                    .InstanceMask = 1,
                    .TransformMatrix = transformMatrix,
                    .BlasResource = meshPrimitiveData->RaytracingAccelerationStructure
                };

                /*
            applicationPayload->ShaderParameters.Scale = sceneNode->Scale;
            applicationPayload->ShaderParameters.Translation = sceneNode->Translation;
            applicationPayload->ShaderParameters.Rotation = sceneNode->Rotation;
            */
            }
        }
    }

    ElemDataSpan tlasInstanceData = ElemEncodeRaytracingTlasInstances((ElemRaytracingTlasInstanceSpan) { .Items = tlasInstances, .Length = tlasInstanceCount });
    ElemUploadGraphicsBufferData(applicationPayload->TestSceneData.TlasInstanceBuffer.Buffer, 0, tlasInstanceData);

    ElemRaytracingTlasParameters tlasParameters =
    {
        .BuildFlags = ElemRaytracingBuildFlags_PreferFastTrace,
        .InstanceBuffer = applicationPayload->TestSceneData.TlasInstanceBuffer.Buffer,
        .InstanceCount = tlasInstanceCount,
    };

    ElemBuildRaytracingTlas(commandList, sceneData->RaytracingAccelerationStructure, sceneData->RaytracingScratchBuffer.Buffer, &tlasParameters, NULL);
}

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->Window = ElemCreateWindow(&(ElemWindowOptions) { .WindowState = applicationPayload->AppSettings.PreferFullScreen ? ElemWindowState_FullScreen : ElemWindowState_Normal });

    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = !applicationPayload->AppSettings.DisableDiagnostics, .EnableGpuValidation = false, .EnableDebugBarrierInfo = false, .PreferVulkan = applicationPayload->AppSettings.PreferVulkan });
    
    // TODO: Debug why the AMD integrated GPU is not create at all
    ElemGraphicsDeviceInfoSpan devices = ElemGetAvailableGraphicsDevices();
    
    for (uint32_t i = 0; i < devices.Length; i++)
    {
        printf("Device: %s\n", devices.Items[i].DeviceName);
    }

    applicationPayload->GraphicsDevice = ElemCreateGraphicsDevice(NULL);

    applicationPayload->CommandQueue= ElemCreateCommandQueue(applicationPayload->GraphicsDevice, ElemCommandQueueType_Graphics, NULL);
    applicationPayload->SwapChain= ElemCreateSwapChain(applicationPayload->CommandQueue, applicationPayload->Window, UpdateSwapChain, &(ElemSwapChainOptions) { .FrameLatency = 1, .UpdatePayload = payload });
    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(applicationPayload->SwapChain);

    // TODO: For now we create a separate heap to avoid memory management
    applicationPayload->DepthBufferHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_Gpu });
    applicationPayload->GpuMemory = SampleCreateGpuMemory(applicationPayload->GraphicsDevice, ElemGraphicsHeapType_Gpu, SampleMegaBytesToBytes(256));
    applicationPayload->GpuMemoryUpload = SampleCreateGpuMemory(applicationPayload->GraphicsDevice, ElemGraphicsHeapType_GpuUpload, SampleMegaBytesToBytes(256));

    CreateDepthBuffer(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);
    CreateRenderTarget(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);
    SampleLoadScene("CornellBox.scene", &applicationPayload->TestSceneData, &applicationPayload->GpuMemoryUpload);
    
    applicationPayload->ShaderGlobalParametersBuffer = SampleCreateGpuBufferAndUploadData(&applicationPayload->GpuMemoryUpload, &applicationPayload->ShaderGlobalParameters, sizeof(ShaderShaderGlobalParameters), "ShaderGlobalParameters");
    applicationPayload->ShaderParameters.ShaderGlobalParametersBuffer = applicationPayload->ShaderGlobalParametersBuffer.ReadDescriptor;

    ElemCommandList loadDataCommandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL);
    
    for (uint32_t i = 0; i < applicationPayload->TestSceneData.NodeCount; i++)
    {
        SampleSceneNodeHeader* sceneNode = &applicationPayload->TestSceneData.Nodes[i];

        if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
        {
            SampleMeshData* meshData = &applicationPayload->TestSceneData.Meshes[sceneNode->ReferenceIndex];
            SampleLoadMeshData(loadDataCommandList, meshData, &applicationPayload->GpuMemoryUpload);
        }
    }

    CreateRaytracingAccelerationStructures(applicationPayload);
    BuildRaytracingAccelerationStructures(loadDataCommandList, applicationPayload);

    ElemCommitCommandList(loadDataCommandList);
    ElemExecuteCommandList(applicationPayload->CommandQueue, loadDataCommandList, NULL);

    ElemDataSpan shaderData = SampleReadFile(!applicationPayload->AppSettings.PreferVulkan ? "RenderMesh.shader": "RenderMesh_vulkan.shader", true);
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

    shaderData = SampleReadFile(!applicationPayload->AppSettings.PreferVulkan ? "Raytracing.shader": "Raytracing_vulkan.shader", true);
    shaderLibrary = ElemCreateShaderLibrary(applicationPayload->GraphicsDevice, shaderData);

    applicationPayload->RaytracingGraphicsPipeline = ElemCompileGraphicsPipelineState(applicationPayload->GraphicsDevice, &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "RayTracing PSO",
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {{ .Format = swapChainInfo.Format }}, .Length = 1 },
    });

    ElemFreeShaderLibrary(shaderLibrary);

    SampleInputsApplicationInit(&applicationPayload->InputsApplication);
    SampleInputsCameraInit(&applicationPayload->InputsCamera);

    applicationPayload->InputsCamera.State.Camera.Position = (ElemVector3) { 0.0f, 1.0f, -3.4f };
    applicationPayload->InputsCamera.State.Camera.Rotation = (ElemVector3) { 0.0f, 0.0f, 0 };
    
    if (applicationPayload->AppSettings.PreferFullScreen)
    {
        ElemHideWindowCursor(applicationPayload->Window);
        applicationPayload->InputsApplication.State.IsCursorDisplayed = false;
    }

    SampleStartFrameMeasurement();

    ElemDataSpan savedStateData = SampleReadFile("SavedState.bin", false);

    if (savedStateData.Length > 0)
    {
        applicationPayload->InputsCamera.State = ((SavedState*)savedStateData.Items)->CameraState;
        applicationPayload->InputsCamera.State.ProjectionMatrix = (SampleMatrix4x4){};
    }
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemWaitForFenceOnCpu(applicationPayload->LastExecutionFence);

    SampleFreeGpuTexture(&applicationPayload->RenderTargetTexture);
    SampleFreeScene(&applicationPayload->TestSceneData);
    SampleFreeGpuBuffer(&applicationPayload->ShaderGlobalParametersBuffer);

    ElemFreePipelineState(applicationPayload->GraphicsPipeline);
    ElemFreePipelineState(applicationPayload->RaytracingGraphicsPipeline);
    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
 
    ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    ElemFreeGraphicsHeap(applicationPayload->DepthBufferHeap);

    SampleFreeGpuMemory(&applicationPayload->GpuMemoryUpload);
    SampleFreeGpuMemory(&applicationPayload->GpuMemory);

    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);
    
    SavedState savedState = { .CameraState = applicationPayload->InputsCamera.State }; 
    SampleWriteDataToApplicationFile("SavedState.bin", (ElemDataSpan) { .Items = (uint8_t*)&savedState, .Length = sizeof(SavedState) }, false);

    printf("Exit application...\n");
}

void UpdateShaderGlobalParameters(ApplicationPayload* applicationPayload, const SampleInputsCameraState* cameraState)
{
    applicationPayload->ShaderGlobalParameters.ViewProjMatrix = cameraState->ViewProjMatrix;
    applicationPayload->ShaderGlobalParameters.InverseViewMatrix = cameraState->InverseViewMatrix;
    applicationPayload->ShaderGlobalParameters.InverseProjectionMatrix = cameraState->InverseProjectionMatrix;
    applicationPayload->ShaderGlobalParameters.MaterialBufferIndex = applicationPayload->TestSceneData.MaterialBuffer.ReadDescriptor;
    applicationPayload->ShaderGlobalParameters.GpuMeshInstanceBufferIndex = applicationPayload->TestSceneData.GpuMeshInstanceBuffer.ReadDescriptor;
    applicationPayload->ShaderGlobalParameters.Action = cameraState->Action;

    ElemUploadGraphicsBufferData(applicationPayload->ShaderGlobalParametersBuffer.Buffer, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderGlobalParameters, .Length = sizeof(ShaderShaderGlobalParameters) });
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    
    if (updateParameters->SizeChanged)
    {
        CreateDepthBuffer(applicationPayload, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);
        CreateRenderTarget(applicationPayload, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);
    }

    ElemInputStream inputStream = ElemGetInputStream();

    SampleInputsApplicationUpdate(inputStream, &applicationPayload->InputsApplication, updateParameters->DeltaTimeInSeconds);
    SampleInputsCameraUpdate(inputStream, &applicationPayload->InputsCamera, updateParameters);

    if (applicationPayload->InputsApplication.State.ExitApplication)
    {
        ElemExitApplication(0);
    }

    if (applicationPayload->InputsApplication.State.ShowCursor)
    {
        printf("Show cursor\n");
        ElemShowWindowCursor(applicationPayload->Window);
    }
    else if (applicationPayload->InputsApplication.State.HideCursor)
    {
        printf("Hide cursor\n");
        ElemHideWindowCursor(applicationPayload->Window); 
    } 

    SampleInputsCameraState* inputsCameraState = &applicationPayload->InputsCamera.State;
    UpdateShaderGlobalParameters(applicationPayload, inputsCameraState);

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

    if (inputsCameraState->Action)
    {
        ElemBindPipelineState(commandList, applicationPayload->RaytracingGraphicsPipeline); 
    
        RaytracingShaderParameters parameters = 
        {
            .MaterialBuffer = applicationPayload->TestSceneData.MaterialBuffer.ReadDescriptor,
            .AccelerationStructureIndex = applicationPayload->TestSceneData.RaytracingAccelerationStructureReadDescriptor,
            .ShaderGlobalParametersBufferIndex = applicationPayload->ShaderGlobalParametersBuffer.ReadDescriptor
        };

        ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&parameters, .Length = sizeof(RaytracingShaderParameters) });
        ElemDispatchMesh(commandList, 1, 1, 1);
    }
    else
    {
        ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline); 
        ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });

        uint32_t currentMeshIndexId = 0;
        for (uint32_t i = 0; i < applicationPayload->TestSceneData.NodeCount; i++)
        {
            SampleSceneNodeHeader* sceneNode = &applicationPayload->TestSceneData.Nodes[i];

            if (sceneNode->NodeType == SampleSceneNodeType_Mesh)
            {
                SampleMeshData* meshData = &applicationPayload->TestSceneData.Meshes[sceneNode->ReferenceIndex];

                for (uint32_t j = 0; j < meshData->MeshHeader.MeshPrimitiveCount; j++)
                {
                    SampleMeshPrimitiveData* meshPrimitive = &meshData->MeshPrimitives[j];
                    applicationPayload->ShaderParameters.PrimitiveId = j;
                    applicationPayload->ShaderParameters.MeshInstanceId = currentMeshIndexId;

                    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });
                    ElemDispatchMesh(commandList, meshPrimitive->PrimitiveHeader.MeshletCount, 1, 1);
                }

                currentMeshIndexId++;
            }
        }
    }

    ElemEndRenderPass(commandList);
    ElemCommitCommandList(commandList);
    applicationPayload->LastExecutionFence = ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, NULL);

    ElemPresentSwapChain(applicationPayload->SwapChain);
    SampleFrameMeasurement frameMeasurement = SampleEndFrameMeasurement();

    if (frameMeasurement.HasNewData)
    {
        SampleSetWindowTitle(applicationPayload->Window, "Hello Raytracing", applicationPayload->GraphicsDevice, frameMeasurement.FrameTimeInSeconds, frameMeasurement.Fps);
    }
    
    SampleStartFrameMeasurement();
}

int main(int argc, const char* argv[]) 
{
    ApplicationPayload payload =
    {
        .AppSettings = SampleParseAppSettings(argc, argv)
    };

    ElemConfigureLogHandler(ElemConsoleLogHandler);

    ElemRunApplication(&(ElemRunApplicationParameters)
    {
        .ApplicationName = "Hello Raytracing",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });
}
