#include "Elemental.h"
#include "SampleUtils.h"
#include "SampleMath.h"
#include "SampleInputsApplication.h"
#include "SampleInputsCamera.h"
#include "SampleSceneLoader.h"
#include "SampleGpuMemory.h"
#include "SampleGpuScene.h"
#include "SampleRaytracingScene.h"
#include "SampleShader.h"

#include "DebugUI.h"

#include "Data/ShaderData.h"

// TODO: Group common variables into separate structs

typedef struct
{
    const char* Label;
    uint32_t Level;
    ElemGraphicsTimestamp StartGpuTimestamp;
    ElemGraphicsTimestamp EndGpuTimestamp;
    float Value;
    bool IsActive;
} GpuTimestamp;

typedef struct
{
    union
    {
        GpuTimestamp RawData[32];
        
        struct
        {
            GpuTimestamp GpuTimestamp;
            GpuTimestamp RenderingTimestamp;
            GpuTimestamp PathtracingTimestamp;
            GpuTimestamp UITimestamp;
            GpuTimestamp TonemapTimestamp;
        };
    };
    uint32_t Count;
} GpuTimestampData;

typedef struct
{
    SampleAppSettings AppSettings;
    const char* ScenePath;

    ElemWindow Window;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;

    SampleGpuMemory GpuMemory;
    SampleGpuMemory GpuMemoryUpload;

    ElemFence LastExecutionFence;
    ElemSwapChain SwapChain;

    ElemGraphicsHeap RenderTargetHeap;
    // TODO: Convert that to sample texture
    ElemGraphicsResource RenderTargetTexture;
    ElemGraphicsResourceDescriptor RenderTargetTextureReadDescriptor;
    ElemGraphicsResourceDescriptor RenderTargetTextureWriteDescriptor;
    
    ElemGraphicsHeap DepthBufferHeap;
    ElemGraphicsResource DepthBuffer;
    
    SampleShader GraphicsPipeline;
    SampleShader PathTracingGraphicsPipeline; // TODO: Rename
    SampleShader ToneMapGraphicsPipeline;
    SampleShader DrawRenderTargetGraphicsPipeline;

    ShaderParameters ShaderParameters;
    SampleInputsApplication InputsApplication;
    SampleInputsCamera InputsCamera;
    SampleSceneData TestSceneData; // TODO: Do we keep that structure here?

    ShaderGlobalParameters ShaderGlobalParameters;
    SampleGpuBuffer ShaderGlobalParametersBuffer;

    SampleGpuSceneData GpuSceneData;
    SampleRaytracingSceneData RaytracingSceneData;

    uint32_t RenderTargetSampleCount;
    bool UsePathTracing;
    bool UsePathTracingAccumulation;
    uint32_t PathTraceLength;

    DebugUIData DebugUIData;
    GpuTimestampData GpuTimestampData;
} ApplicationPayload;

typedef struct
{
    uint32_t SourceTexture;
    uint32_t SampleCount;
} ToneMapShaderParameters;

typedef struct
{
    SampleInputsCameraState CameraState; 
} SavedState;
    

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

void CreateRenderTarget(ApplicationPayload* applicationPayload, uint32_t width, uint32_t height)
{
    if (applicationPayload->RenderTargetTexture != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsResourceDescriptor(applicationPayload->RenderTargetTextureReadDescriptor, NULL);
        ElemFreeGraphicsResourceDescriptor(applicationPayload->RenderTargetTextureWriteDescriptor, NULL);
        ElemFreeGraphicsResource(applicationPayload->RenderTargetTexture, NULL);
    }

    printf("Creating render texture...\n");

    ElemGraphicsResourceInfo resourceInfo = ElemCreateTexture2DResourceInfo(applicationPayload->GraphicsDevice, width, height, 1, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget | ElemGraphicsResourceDescriptorUsage_Write,
                                                                            &(ElemGraphicsResourceInfoOptions) { 
                                                                                .DebugName = "FloatRenderTarget" 
                                                                            });

    applicationPayload->RenderTargetTexture = ElemCreateGraphicsResource(applicationPayload->RenderTargetHeap, 0, &resourceInfo);
    applicationPayload->RenderTargetTextureReadDescriptor = ElemCreateGraphicsResourceDescriptor(applicationPayload->RenderTargetTexture, ElemGraphicsResourceDescriptorUsage_Read, NULL);
    applicationPayload->RenderTargetTextureWriteDescriptor = ElemCreateGraphicsResourceDescriptor(applicationPayload->RenderTargetTexture, ElemGraphicsResourceDescriptorUsage_Write, NULL);
}

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

void UpdateShaderGlobalParameters(ApplicationPayload* applicationPayload, const SampleInputsCameraState* cameraState)
{
    applicationPayload->ShaderGlobalParameters.ViewProjMatrix = cameraState->ViewProjMatrix;
    applicationPayload->ShaderGlobalParameters.InverseViewMatrix = cameraState->InverseViewMatrix;
    applicationPayload->ShaderGlobalParameters.InverseProjectionMatrix = cameraState->InverseProjectionMatrix;
    applicationPayload->ShaderGlobalParameters.MaterialBufferIndex = applicationPayload->GpuSceneData.MaterialBuffer.ReadDescriptor;
    applicationPayload->ShaderGlobalParameters.MeshInstanceBufferIndex = applicationPayload->GpuSceneData.MeshInstanceBuffer.ReadDescriptor;
    applicationPayload->ShaderGlobalParameters.MeshPrimitiveInstanceBufferIndex = applicationPayload->GpuSceneData.MeshPrimitiveInstanceBuffer.ReadDescriptor;

    if (cameraState->Action)
    {
        //applicationPayload->ShaderGlobalParameters.Action = !applicationPayload->ShaderGlobalParameters.Action;
    }

    ElemUploadGraphicsBufferData(applicationPayload->ShaderGlobalParametersBuffer.Buffer, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderGlobalParameters, .Length = sizeof(ShaderGlobalParameters) });
}

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    applicationPayload->Window = ElemCreateWindow(&(ElemWindowOptions) { .WindowState = applicationPayload->AppSettings.PreferFullScreen ? ElemWindowState_FullScreen : ElemWindowState_Normal });

    ElemSetGraphicsOptions(&(ElemGraphicsOptions) { .EnableDebugLayer = !applicationPayload->AppSettings.DisableDiagnostics, .EnableGpuValidation = false, .EnableDebugBarrierInfo = false, .PreferVulkan = applicationPayload->AppSettings.PreferVulkan });
    
    applicationPayload->GraphicsDevice = ElemCreateGraphicsDevice(NULL);

    applicationPayload->CommandQueue= ElemCreateCommandQueue(applicationPayload->GraphicsDevice, ElemCommandQueueType_Graphics, NULL);
    applicationPayload->SwapChain= ElemCreateSwapChain(applicationPayload->CommandQueue, applicationPayload->Window, UpdateSwapChain, &(ElemSwapChainOptions) { .FrameLatency = 1, .UpdatePayload = payload });
    ElemSwapChainInfo swapChainInfo = ElemGetSwapChainInfo(applicationPayload->SwapChain);

    // TODO: For now we create a separate heap to avoid memory management
    applicationPayload->RenderTargetHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(128), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_Gpu });
    applicationPayload->DepthBufferHeap = ElemCreateGraphicsHeap(applicationPayload->GraphicsDevice, SampleMegaBytesToBytes(64), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_Gpu });

    // TODO: For now we need to put the heap as GpuUpload but it should be Gpu when we use IOQueues
    // TODO: Having GPU Upload is still annoying 😞
    // TODO: For the moment we implement all textures of the scene in GPU memory. That is why we need 2GB for bistro scene
    // We will implement virtual texturing/texture streaming in the future
    applicationPayload->GpuMemory = SampleCreateGpuMemory(applicationPayload->GraphicsDevice, ElemGraphicsHeapType_Gpu, SampleMegaBytesToBytes(2048));
    applicationPayload->GpuMemoryUpload = SampleCreateGpuMemory(applicationPayload->GraphicsDevice, ElemGraphicsHeapType_GpuUpload, SampleMegaBytesToBytes(128));

    applicationPayload->ShaderGlobalParametersBuffer = SampleCreateGpuBuffer(&applicationPayload->GpuMemoryUpload, sizeof(ShaderGlobalParameters), ElemGraphicsResourceUsage_Write, "ShaderGlobalParameters");
    applicationPayload->ShaderParameters.GlobalParametersBufferIndex = applicationPayload->ShaderGlobalParametersBuffer.ReadDescriptor;

    // TODO: Do we need the scene data after that?
    CreateRenderTarget(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);
    CreateDepthBuffer(applicationPayload, swapChainInfo.Width, swapChainInfo.Height);
    SampleLoadScene(applicationPayload->ScenePath, &applicationPayload->TestSceneData);

    ElemGraphicsSamplerInfo samplerInfo =
    {
        .MinFilter = ElemGraphicsSamplerFilter_Linear,
        .MagFilter = ElemGraphicsSamplerFilter_Linear,
        .MipFilter = ElemGraphicsSamplerFilter_Linear,
        .MaxAnisotropy = 16,
    };

    applicationPayload->ShaderGlobalParameters.TextureSampler = ElemCreateGraphicsSampler(applicationPayload->GraphicsDevice, &samplerInfo);

    applicationPayload->GraphicsPipeline = SampleCompileGraphicsShader(applicationPayload->GraphicsDevice, "RenderMesh.shader", &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "RenderMesh PSO",
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        //.CullMode = ElemGraphicsCullMode_None, // TODO: We need to deactivate cull only for transparent objects!
        .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {{ .Format = ElemGraphicsFormat_R32G32B32A32_FLOAT }}, .Length = 1 },
        .DepthStencil =
        {
            .Format = ElemGraphicsFormat_D32_FLOAT,
            .DepthCompareFunction = ElemGraphicsCompareFunction_Greater
        }
    });

    applicationPayload->PathTracingGraphicsPipeline = SampleCompileComputeShader(applicationPayload->GraphicsDevice, "PathTracing.shader", &(ElemComputePipelineStateParameters) {
        .DebugName = "PathTracing PSO",
        .ComputeShaderFunction = "PathTracing"
    });

    applicationPayload->ToneMapGraphicsPipeline = SampleCompileGraphicsShader(applicationPayload->GraphicsDevice, "Tonemap.shader", &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "Tonemap PSO",
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {
        { 
            .Format = swapChainInfo.Format,
            .BlendOperation = ElemGraphicsBlendOperation_Add,
            .SourceBlendFactor = ElemGraphicsBlendFactor_SourceAlpha,
            .DestinationBlendFactor = ElemGraphicsBlendFactor_InverseSourceAlpha,
        }}, .Length = 1 },
    });

    applicationPayload->DrawRenderTargetGraphicsPipeline = SampleCompileGraphicsShader(applicationPayload->GraphicsDevice, "DrawRenderTarget.shader", &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "DrawRenderTarget PSO",
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {
        { 
            .Format = swapChainInfo.Format,
            .BlendOperation = ElemGraphicsBlendOperation_Add,
            .SourceBlendFactor = ElemGraphicsBlendFactor_SourceAlpha,
            .DestinationBlendFactor = ElemGraphicsBlendFactor_InverseSourceAlpha,
        }}, .Length = 1 },
    });

    InitDebugUI(applicationPayload->GraphicsDevice, swapChainInfo.Width, swapChainInfo.Height, swapChainInfo.UIScale, &applicationPayload->DebugUIData);

    SampleInputsApplicationInit(&applicationPayload->InputsApplication);
    SampleInputsCameraInit(&applicationPayload->InputsCamera);
    
    if (applicationPayload->AppSettings.PreferFullScreen)
    {
        ElemHideWindowCursor(applicationPayload->Window);
        applicationPayload->InputsApplication.State.IsCursorDisplayed = false;
    }

    applicationPayload->RenderTargetSampleCount = 1;
    applicationPayload->UsePathTracingAccumulation = true;
    applicationPayload->PathTraceLength = 3;

    SampleStartFrameMeasurement();

    ElemDataSpan savedStateData = SampleReadFile("SavedState.bin", false);

    if (savedStateData.Length > 0)
    {
        applicationPayload->InputsCamera.State = ((SavedState*)savedStateData.Items)->CameraState;
        applicationPayload->InputsCamera.State.ProjectionMatrix = (SampleMatrix4x4){};
    }

    applicationPayload->GpuTimestampData.Count = (uint32_t)(&applicationPayload->GpuTimestampData.TonemapTimestamp - &applicationPayload->GpuTimestampData.RawData[0]) + 1;

    // TODO: Refactor that
    applicationPayload->GpuTimestampData.GpuTimestamp = (GpuTimestamp)
    {
        .Label = "Gpu",
        .StartGpuTimestamp = ElemCreateGraphicsTimestamp(applicationPayload->GraphicsDevice),
        .EndGpuTimestamp = ElemCreateGraphicsTimestamp(applicationPayload->GraphicsDevice) 
    };

    applicationPayload->GpuTimestampData.RenderingTimestamp = (GpuTimestamp)
    {
        .Label = "Rendering",
        .Level = 1,
        .StartGpuTimestamp = ElemCreateGraphicsTimestamp(applicationPayload->GraphicsDevice),
        .EndGpuTimestamp = ElemCreateGraphicsTimestamp(applicationPayload->GraphicsDevice) 
    };

    applicationPayload->GpuTimestampData.PathtracingTimestamp = (GpuTimestamp)
    {
        .Label = "Pathtracing",
        .Level = 1,
        .StartGpuTimestamp = ElemCreateGraphicsTimestamp(applicationPayload->GraphicsDevice),
        .EndGpuTimestamp = ElemCreateGraphicsTimestamp(applicationPayload->GraphicsDevice) 
    };

    applicationPayload->GpuTimestampData.UITimestamp = (GpuTimestamp)
    {
        .Label = "UI Rendering",
        .Level = 1,
        .StartGpuTimestamp = ElemCreateGraphicsTimestamp(applicationPayload->GraphicsDevice),
        .EndGpuTimestamp = ElemCreateGraphicsTimestamp(applicationPayload->GraphicsDevice) 
    };

    applicationPayload->GpuTimestampData.TonemapTimestamp = (GpuTimestamp)
    {
        .Label = "Tonemap",
        .Level = 1,
        .StartGpuTimestamp = ElemCreateGraphicsTimestamp(applicationPayload->GraphicsDevice),
        .EndGpuTimestamp = ElemCreateGraphicsTimestamp(applicationPayload->GraphicsDevice) 
    };
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemWaitForFenceOnCpu(applicationPayload->LastExecutionFence);

    SampleFreeGpuSceneData(&applicationPayload->GpuSceneData);
    SampleFreeRaytracingSceneData(&applicationPayload->RaytracingSceneData);
    SampleFreeScene(&applicationPayload->TestSceneData);

    SampleFreeGpuBuffer(&applicationPayload->ShaderGlobalParametersBuffer);

    SampleFreeShader(&applicationPayload->GraphicsPipeline);
    SampleFreeShader(&applicationPayload->PathTracingGraphicsPipeline);
    SampleFreeShader(&applicationPayload->ToneMapGraphicsPipeline);

    ElemFreeSwapChain(applicationPayload->SwapChain);
    ElemFreeCommandQueue(applicationPayload->CommandQueue);
 
    ElemFreeGraphicsSampler(applicationPayload->ShaderGlobalParameters.TextureSampler, NULL);
    ElemFreeGraphicsResource(applicationPayload->DepthBuffer, NULL);
    ElemFreeGraphicsHeap(applicationPayload->DepthBufferHeap);

    ElemFreeGraphicsResourceDescriptor(applicationPayload->RenderTargetTextureReadDescriptor, NULL);
    ElemFreeGraphicsResourceDescriptor(applicationPayload->RenderTargetTextureWriteDescriptor, NULL);
    ElemFreeGraphicsResource(applicationPayload->RenderTargetTexture, NULL);
    ElemFreeGraphicsHeap(applicationPayload->RenderTargetHeap);

    SampleFreeGpuMemory(&applicationPayload->GpuMemoryUpload);
    SampleFreeGpuMemory(&applicationPayload->GpuMemory);
    ElemFreeGraphicsDevice(applicationPayload->GraphicsDevice);

    SavedState savedState = { .CameraState = applicationPayload->InputsCamera.State }; 
    SampleWriteDataToApplicationFile("SavedState.bin", (ElemDataSpan) { .Items = (uint8_t*)&savedState, .Length = sizeof(SavedState) }, false);

    printf("Exit application...\n");
}

uint32_t test = 0;
SampleFrameMeasurement globalFrameMeasurement;

void UpdateGpuTimestampValue(GpuTimestamp* gpuTimestamp)
{ 
    ElemGraphicsTimestampValue startGpuTimestampValue = ElemGetGraphicsTimestampValue(gpuTimestamp->StartGpuTimestamp);

    if (startGpuTimestampValue.FrequencyInSeconds > 0)
    {
        ElemGraphicsTimestampValue endGpuTimestampValue = ElemGetGraphicsTimestampValue(gpuTimestamp->EndGpuTimestamp);
        float rawValue = ((float)(endGpuTimestampValue.Value - startGpuTimestampValue.Value) / startGpuTimestampValue.FrequencyInSeconds) * 1000.0f;

        gpuTimestamp->Value = gpuTimestamp->Value * 0.95f + rawValue * 0.05f;
    }
}

void UpdateDebugUIStatistics(DebugUIData* debugUIData, const SampleFrameMeasurement* frameMeasurement, GpuTimestampData* gpuTimestampData)
{
    // TODO: Fill correct monitor refresh rate
    PushDebugUIStatisticItem(debugUIData, (DebugUIStatisticItem) { .Label = "Fps", .Value = frameMeasurement->Fps, .ExpectedValue = 120, .ExpectedDifferenceGoodRange = 0.99f, .Type = DebugUIStatisticType_Integer });
    PushDebugUIStatisticItem(debugUIData, (DebugUIStatisticItem) { .Label = "Cpu", .Value = frameMeasurement->FrameTimeInSeconds * 1000.0f, .ExpectedValue = 1000.0f / 120.0f, .Type = DebugUIStatisticType_Milliseconds });

    for (uint32_t i = 0; i < gpuTimestampData->Count; i++)
    {
        GpuTimestamp* timestamp = &gpuTimestampData->RawData[i];

        if (!timestamp->IsActive)
        {
            continue;
        }

        // TODO: For the colors, we need to see if the parents is bad and then if the percentage is high higlight in child
        UpdateGpuTimestampValue(timestamp);
        PushDebugUIStatisticItem(debugUIData, (DebugUIStatisticItem) { .Label = timestamp->Label, .Value = timestamp->Value, .Level = timestamp->Level, .ExpectedValue = 1000.0f / 120.0f, .Type = DebugUIStatisticType_Milliseconds, .Group = DebugUIStatisticGroup_GpuPipeline });

        timestamp->IsActive = false;
    }
}

void UpdateSwapChain(const ElemSwapChainUpdateParameters* updateParameters, void* payload)
{
    SampleStartFrameMeasurement();
    test++;
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    
    if (updateParameters->SizeChanged)
    {
        CreateDepthBuffer(applicationPayload, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);
        CreateRenderTarget(applicationPayload, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);

        ResizeDebugUI(&applicationPayload->DebugUIData, updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height);
    }

    if (updateParameters->SizeChanged || applicationPayload->InputsCamera.State.HasChanged || applicationPayload->InputsCamera.State.Action || !applicationPayload->UsePathTracingAccumulation)
    {
        applicationPayload->RenderTargetSampleCount = 0;
    }
    
    UpdateDebugUIStatistics(&applicationPayload->DebugUIData, &globalFrameMeasurement, &applicationPayload->GpuTimestampData);
    
    ElemInputStream inputStream = ElemGetInputStream();

    SampleInputsApplicationUpdate(inputStream, &applicationPayload->InputsApplication, updateParameters->DeltaTimeInSeconds);
    SampleInputsCameraUpdate(inputStream, &applicationPayload->InputsCamera, updateParameters);

    // TODO: We shold move this into the application update function (we can pass the window if needed)
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

    if (inputsCameraState->Action)
    {
        applicationPayload->UsePathTracing = !applicationPayload->UsePathTracing;
    }

    if (inputsCameraState->Action2)
    {
        applicationPayload->UsePathTracingAccumulation = !applicationPayload->UsePathTracingAccumulation;
    }
    // TODO: We need to have a kind of queue system. The problem here is that if we don't have any
    // data to load we will create empty lists

    ElemFence loadDataFence = {};

    if (!applicationPayload->GpuSceneData.IsLoaded)
    {
        // TODO: We should have a queue system instead and load only what is needed otherwise
        // each frames we will have empty lists
        // This system will allow to split by batches the loading
        ElemCommandList loadDataCommandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL);

        SampleCreateGpuSceneData(loadDataCommandList, &applicationPayload->TestSceneData, &applicationPayload->GpuSceneData, &applicationPayload->GpuMemory);
        SampleCreateRaytracingSceneData(applicationPayload->GraphicsDevice, loadDataCommandList, &applicationPayload->TestSceneData, &applicationPayload->GpuSceneData, &applicationPayload->RaytracingSceneData, &applicationPayload->GpuMemory, &applicationPayload->GpuMemoryUpload);

        ElemCommitCommandList(loadDataCommandList);    // TODO: Measure scene loading time 
        loadDataFence = ElemExecuteCommandList(applicationPayload->CommandQueue, loadDataCommandList, NULL);
    }

    ElemCommandList commandList = ElemGetCommandList(applicationPayload->CommandQueue, NULL); 

    // TODO: Refactor that
    ElemInsertGraphicsTimestamp(commandList, applicationPayload->GpuTimestampData.GpuTimestamp.StartGpuTimestamp);
    applicationPayload->GpuTimestampData.GpuTimestamp.IsActive = true;

    if (!applicationPayload->UsePathTracing)
    {
        // TODO: Refactor that
        ElemInsertGraphicsTimestamp(commandList, applicationPayload->GpuTimestampData.RenderingTimestamp.StartGpuTimestamp);
        applicationPayload->GpuTimestampData.RenderingTimestamp.IsActive = true;

        ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters) {
            .RenderTargets = 
            {
                .Items = (ElemRenderPassRenderTarget[]) { 
                {
                    .RenderTarget = applicationPayload->RenderTargetTexture,
                    .LoadAction = ElemRenderPassLoadAction_Clear,
                }},
                .Length = 1
            },
            .DepthStencil =
            {
                .DepthStencil = applicationPayload->DepthBuffer
            }
        });


        applicationPayload->RenderTargetSampleCount = 1;
        ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline.PipelineState); 

        // TODO: Construct a list of tasks on the cpu for now and do only one dispatch mesh with the total of tasks
        // Be carreful with the limit per dimension of 65000
        ElemBindPipelineState(commandList, applicationPayload->GraphicsPipeline.PipelineState); 
        ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });

        for (uint32_t i = 0; i < applicationPayload->GpuSceneData.MeshPrimitiveInstanceCount; i++)
        {
            applicationPayload->ShaderParameters.AccelerationStructureIndex = applicationPayload->RaytracingSceneData.TlasReadDescriptor;
            applicationPayload->ShaderParameters.MeshPrimitiveInstanceId = i;

            ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&applicationPayload->ShaderParameters, .Length = sizeof(ShaderParameters) });
            ElemDispatchMesh(commandList, applicationPayload->GpuSceneData.MeshPrimitiveMeshletCountList[i], 1, 1);
        }
    
        ElemEndRenderPass(commandList);
        ElemInsertGraphicsTimestamp(commandList, applicationPayload->GpuTimestampData.RenderingTimestamp.EndGpuTimestamp);
    }
    else
    {
        // TODO: Refactor that
        ElemInsertGraphicsTimestamp(commandList, applicationPayload->GpuTimestampData.PathtracingTimestamp.StartGpuTimestamp);
        applicationPayload->GpuTimestampData.PathtracingTimestamp.IsActive = true;

        ElemGraphicsResourceBarrier(commandList, applicationPayload->RenderTargetTextureWriteDescriptor, NULL);

        applicationPayload->RenderTargetSampleCount++;
        ElemBindPipelineState(commandList, applicationPayload->PathTracingGraphicsPipeline.PipelineState); 
    
        RaytracingShaderParameters parameters = 
        {
            .AccelerationStructureIndex = applicationPayload->RaytracingSceneData.TlasReadDescriptor,
            .GlobalParametersBufferIndex = applicationPayload->ShaderGlobalParametersBuffer.ReadDescriptor,
            .OutputTextureIndex = applicationPayload->RenderTargetTextureWriteDescriptor,
            .OutputTextureSize = { updateParameters->SwapChainInfo.Width, updateParameters->SwapChainInfo.Height },
            // TODO: To Replace
            //.FrameIndex = updateParameters->FrameIndex
            .FrameIndex = test,
            .SampleCount = applicationPayload->RenderTargetSampleCount,
            .PathTraceLength = applicationPayload->PathTraceLength
        };

        ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&parameters, .Length = sizeof(RaytracingShaderParameters) });

        uint32_t threadSize = 8;
        ElemDispatchCompute(commandList, (updateParameters->SwapChainInfo.Width + (threadSize - 1)) / threadSize, (updateParameters->SwapChainInfo.Height + (threadSize - 1)) / threadSize, 1);
        ElemInsertGraphicsTimestamp(commandList, applicationPayload->GpuTimestampData.PathtracingTimestamp.EndGpuTimestamp);
    }

    // TODO: Refactor that
    ElemInsertGraphicsTimestamp(commandList, applicationPayload->GpuTimestampData.UITimestamp.StartGpuTimestamp);
    applicationPayload->GpuTimestampData.UITimestamp.IsActive = true;

    RenderDebugUI(commandList, &applicationPayload->DebugUIData); 
    
    ElemInsertGraphicsTimestamp(commandList, applicationPayload->GpuTimestampData.UITimestamp.EndGpuTimestamp);

    ElemGraphicsResourceBarrier(commandList, applicationPayload->RenderTargetTextureReadDescriptor, NULL);
    ElemGraphicsResourceBarrier(commandList, applicationPayload->DebugUIData.UIRenderTargetTextureReadDescriptor, NULL);

    // TODO: Refactor that
    ElemInsertGraphicsTimestamp(commandList, applicationPayload->GpuTimestampData.TonemapTimestamp.StartGpuTimestamp);
    applicationPayload->GpuTimestampData.TonemapTimestamp.IsActive = true;

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
        
    ToneMapShaderParameters parameters = 
    {
        .SourceTexture = applicationPayload->RenderTargetTextureReadDescriptor,
        .SampleCount = applicationPayload->RenderTargetSampleCount
    };

    ElemBindPipelineState(commandList, applicationPayload->ToneMapGraphicsPipeline.PipelineState); 
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&parameters, .Length = sizeof(ToneMapShaderParameters) });
    ElemDispatchMesh(commandList, 1, 1, 1);

    // TODO: Change the parameter type
    parameters =  (ToneMapShaderParameters)
    {
        .SourceTexture = applicationPayload->DebugUIData.UIRenderTargetTextureReadDescriptor,
        .SampleCount = applicationPayload->ShaderGlobalParameters.TextureSampler
    };

    ElemBindPipelineState(commandList, applicationPayload->DrawRenderTargetGraphicsPipeline.PipelineState); 
    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&parameters, .Length = sizeof(ToneMapShaderParameters) });
    ElemDispatchMesh(commandList, 1, 1, 1);

    ElemEndRenderPass(commandList);
    
    ElemInsertGraphicsTimestamp(commandList, applicationPayload->GpuTimestampData.TonemapTimestamp.EndGpuTimestamp);
    ElemInsertGraphicsTimestamp(commandList, applicationPayload->GpuTimestampData.GpuTimestamp.EndGpuTimestamp);
    ElemCommitCommandList(commandList);

    ElemExecuteCommandListOptions executeOptions = {};

    if (loadDataFence.CommandQueue != ELEM_HANDLE_NULL)
    {
        // BUG: Sometime we have a crash on vulkan
        executeOptions.FencesToWait = (ElemFenceSpan){ .Items = &loadDataFence, .Length = 1 };
    }

    applicationPayload->LastExecutionFence = ElemExecuteCommandList(applicationPayload->CommandQueue, commandList, &executeOptions);

    ElemPresentSwapChain(applicationPayload->SwapChain);
    SampleFrameMeasurement frameMeasurement = SampleEndFrameMeasurement();
    globalFrameMeasurement = frameMeasurement;

    if (frameMeasurement.HasNewData)
    {
        SampleSetWindowTitle(applicationPayload->Window, "Renderer", applicationPayload->GraphicsDevice, frameMeasurement.FrameTimeInSeconds, frameMeasurement.Fps);
    }
}

int main(int argc, const char* argv[]) 
{
    ApplicationPayload payload =
    {
        .AppSettings = SampleParseAppSettings(argc, argv),
        .ScenePath = "Sponza/sponza.scene"
    };

    int32_t scenePathIndex = argc - 1;

    if (strstr(argv[scenePathIndex], ".scene"))
    {
        payload.ScenePath = argv[scenePathIndex];
    }

    ElemConfigureLogHandler(SampleConsoleAndFileLogHandler);

    ElemRunApplication(&(ElemRunApplicationParameters)
    {
        .ApplicationName = "Renderer",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });

}
