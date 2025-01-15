#include "MetalShader.h"
#include "MetalConfig.h"
#include "MetalGraphicsDevice.h"
#include "MetalCommandList.h"
#include "MetalResource.h"
#include "MetalResourceBarrier.h"
#include "Graphics/Resource.h"
#include "Graphics/Shader.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

struct MetalShaderFunctionData
{
    NS::SharedPtr<MTL::Function> Function;
    MetalShaderMetaData MetaData;
};

SystemDataPool<MetalShaderLibraryData, SystemDataPoolDefaultFull> metalShaderLibraryPool;
SystemDataPool<MetalPipelineStateData, MetalPipelineStateDataFull> metalPipelineStatePool;

void InitMetalShaderLibraryMemory()
{
    if (!metalShaderLibraryPool.Storage)
    {
        metalShaderLibraryPool = SystemCreateDataPool<MetalShaderLibraryData>(MetalGraphicsMemoryArena, METAL_MAX_LIBRARIES);
        metalPipelineStatePool = SystemCreateDataPool<MetalPipelineStateData, MetalPipelineStateDataFull>(MetalGraphicsMemoryArena, METAL_MAX_PIPELINESTATES);
    }
}

MetalShaderLibraryData* GetMetalShaderLibraryData(ElemShaderLibrary shaderLibrary)
{
    return SystemGetDataPoolItem(metalShaderLibraryPool, shaderLibrary);
}

MetalPipelineStateData* GetMetalPipelineStateData(ElemPipelineState pipelineState)
{
    return SystemGetDataPoolItem(metalPipelineStatePool, pipelineState);
}

MetalPipelineStateDataFull* GetMetalPipelineStateDataFull(ElemPipelineState pipelineState)
{
    return SystemGetDataPoolItemFull(metalPipelineStatePool, pipelineState);
}

bool CheckMetalShaderDataHeader(ElemDataSpan data, const char* headerValue)
{
    for (uint32_t i = 0; i < 3; i++)
    {
        if (data.Items[i] != headerValue[i])
        {
            return false;
        }
    }

    return true;
}

bool CheckMetalCommandEncoderType(const MetalCommandListData* commandListData, MetalCommandEncoderType type)
{
    if (commandListData->CommandEncoderType != type)
    {
        if (type == MetalCommandEncoderType_Compute)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "A compute pipelinestate must be bound to the commandlist before calling a compute command.");
        }
        else
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "A graphics pipelinestate must be bound to the commandlist before calling a rendering command.");
        }

        return false;
    }

    return true;
}

MetalShaderFunctionData GetMetalShaderFunction(MetalShaderLibraryData* shaderLibraryData, ShaderType shaderType, const char* function)
{
    SystemAssert(function);
    MetalShaderFunctionData result = {};

    for (uint32_t i = 0; i < shaderLibraryData->GraphicsShaders.Length; i++)
    {
        auto shader = shaderLibraryData->Shaders[i];

        if (shader.ShaderType == shaderType && SystemFindSubString(shader.Name, function) != -1)
        {
            result.Function = NS::TransferPtr(shaderLibraryData->GraphicsShaders[i]->newFunction(NS::String::string(function, NS::UTF8StringEncoding)));
            
            for (uint32_t j = 0; j < shader.Metadata.Length; j++)
            {
                auto metaData = shader.Metadata[j];

                if (metaData.Type == ShaderMetadataType_ThreadGroupSize)
                {
                    result.MetaData.ThreadSizeX = metaData.Value[0];
                    result.MetaData.ThreadSizeY = metaData.Value[1];
                    result.MetaData.ThreadSizeZ = metaData.Value[2];
                }
            }

            break;
        }
    }
        
    if (!result.Function)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Cannot find shader function '%s'", function);
    }

    return result;
}

MTL::CullMode ConvertToMetalCullMode(ElemGraphicsCullMode cullMode)
{
    switch (cullMode) 
    {
        case ElemGraphicsCullMode_BackFace:
            return MTL::CullModeBack;

        case ElemGraphicsCullMode_FrontFace:
            return MTL::CullModeFront;

        case ElemGraphicsCullMode_None:
            return MTL::CullModeNone;
    }
}

MTL::BlendOperation ConvertToMetalBlendOperation(ElemGraphicsBlendOperation blendOperation)
{
    switch (blendOperation)
    {
        case ElemGraphicsBlendOperation_Add:
            return MTL::BlendOperationAdd;

        case ElemGraphicsBlendOperation_Subtract:
            return MTL::BlendOperationSubtract;

        case ElemGraphicsBlendOperation_ReverseSubtract:
            return MTL::BlendOperationReverseSubtract;

        case ElemGraphicsBlendOperation_Min:
            return MTL::BlendOperationMin;

        case ElemGraphicsBlendOperation_Max:
            return MTL::BlendOperationMax;
    }
}

MTL::BlendFactor ConvertToMetalBlendFactor(ElemGraphicsBlendFactor blendFactor)
{
    switch (blendFactor)
    {
        case ElemGraphicsBlendFactor_Zero:
            return MTL::BlendFactorZero;

        case ElemGraphicsBlendFactor_One:
            return MTL::BlendFactorOne;

        case ElemGraphicsBlendFactor_SourceColor:
            return MTL::BlendFactorSourceColor;

        case ElemGraphicsBlendFactor_InverseSourceColor:
            return MTL::BlendFactorOneMinusSourceColor;

        case ElemGraphicsBlendFactor_SourceAlpha:
            return MTL::BlendFactorSourceAlpha;

        case ElemGraphicsBlendFactor_InverseSourceAlpha:
            return MTL::BlendFactorOneMinusSourceAlpha;

        case ElemGraphicsBlendFactor_DestinationColor:
            return MTL::BlendFactorDestinationColor;

        case ElemGraphicsBlendFactor_InverseDestinationColor:
            return MTL::BlendFactorOneMinusDestinationColor;

        case ElemGraphicsBlendFactor_DestinationAlpha:
            return MTL::BlendFactorDestinationAlpha;

        case ElemGraphicsBlendFactor_InverseDestinationAlpha:
            return MTL::BlendFactorOneMinusDestinationAlpha;

        case ElemGraphicsBlendFactor_SourceAlphaSaturated:
            return MTL::BlendFactorSourceAlphaSaturated;
    }
}

ElemShaderLibrary MetalCreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData)
{
    InitMetalShaderLibraryMemory();
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    SystemAssert(shaderLibraryData.Length > 0);
    
    NS::SharedPtr<MTL::Library> metalLibrary = {};
    Span<NS::SharedPtr<MTL::Library>> graphicsShaderData = {};
    ReadOnlySpan<Shader> shaders = {};

    if (CheckMetalShaderDataHeader(shaderLibraryData, "MTL"))
    {
        auto dispatchData = dispatch_data_create(shaderLibraryData.Items, shaderLibraryData.Length, nullptr, nullptr);

        NS::Error* errorPointer;
        metalLibrary = NS::TransferPtr(graphicsDeviceData->Device->newLibrary(dispatchData, &errorPointer));
    
        auto libraryError = NS::TransferPtr(errorPointer);

        if (libraryError->code() != 0)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Cannot create shader library. Error Code: %d => %s", libraryError->code(), libraryError->localizedFailureReason()->cString(NS::UTF8StringEncoding));
            return ELEM_HANDLE_NULL;
        }
    }
    else 
    {
        auto dataSpan = Span<uint8_t>(shaderLibraryData.Items, shaderLibraryData.Length); 

        // HACK: This is bad to allocate this here but this should be temporary
        // Here the solution should be to define a memory arena per library (malloc style)
        shaders = ReadShaders(MetalGraphicsMemoryArena, dataSpan);
        graphicsShaderData = SystemPushArray<NS::SharedPtr<MTL::Library>>(MetalGraphicsMemoryArena, shaders.Length);
        
        for (uint32_t i = 0; i < shaders.Length; i++)
        {
            auto dispatchData = dispatch_data_create(shaders[i].ShaderCode.Pointer, shaders[i].ShaderCode.Length, nullptr, nullptr);

            NS::Error* errorPointer;
            graphicsShaderData[i] = NS::TransferPtr(graphicsDeviceData->Device->newLibrary(dispatchData, &errorPointer));
        
            auto libraryError = NS::TransferPtr(errorPointer);

            if (libraryError->code() != 0)
            {
                auto errorMessageNative = libraryError->localizedFailureReason();
                auto errorMessage = "Unknown Error";

                if (errorMessageNative != nullptr)
                {
                    errorMessage = errorMessageNative->cString(NS::UTF8StringEncoding);
                }

                SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Cannot create shader library. Error Code: %d => %s", libraryError->code(), errorMessage);
                return ELEM_HANDLE_NULL;
            }
        }
    }

    auto handle = SystemAddDataPoolItem(metalShaderLibraryPool, {
        .MetalLibrary = metalLibrary,
        .GraphicsShaders = graphicsShaderData,
        .Shaders = shaders
    }); 

    return handle;
}

void MetalFreeShaderLibrary(ElemShaderLibrary shaderLibrary)
{
    // TODO: Free data
}

ElemPipelineState MetalCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters)
{
    InitMetalShaderLibraryMemory();
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    SystemAssert(parameters);
    SystemAssert(parameters->ShaderLibrary != ELEM_HANDLE_NULL);

    auto shaderLibraryData = GetMetalShaderLibraryData(parameters->ShaderLibrary);
    SystemAssert(shaderLibraryData);

    auto pipelineStateDescriptor = NS::TransferPtr(MTL::MeshRenderPipelineDescriptor::alloc()->init());
        
    for (uint32_t i = 0; i < parameters->RenderTargets.Length; i++)
    {
        auto renderTargetParameters = parameters->RenderTargets.Items[i];
        auto metalColorAttachment = pipelineStateDescriptor->colorAttachments()->object(i);

        metalColorAttachment->setPixelFormat(ConvertToMetalResourceFormat(renderTargetParameters.Format));
        
        metalColorAttachment->setBlendingEnabled(IsBlendEnabled(renderTargetParameters));
        metalColorAttachment->setSourceRGBBlendFactor(ConvertToMetalBlendFactor(renderTargetParameters.SourceBlendFactor));
        metalColorAttachment->setDestinationRGBBlendFactor(ConvertToMetalBlendFactor(renderTargetParameters.DestinationBlendFactor));
        metalColorAttachment->setRgbBlendOperation(ConvertToMetalBlendOperation(renderTargetParameters.BlendOperation));
        metalColorAttachment->setSourceAlphaBlendFactor(ConvertToMetalBlendFactor(renderTargetParameters.SourceBlendFactorAlpha));
        metalColorAttachment->setDestinationAlphaBlendFactor(ConvertToMetalBlendFactor(renderTargetParameters.DestinationBlendFactorAlpha));
        metalColorAttachment->setAlphaBlendOperation(ConvertToMetalBlendOperation(renderTargetParameters.BlendOperationAlpha));
        metalColorAttachment->setWriteMask(MTL::ColorWriteMaskAll);
    }

    NS::SharedPtr<MTL::DepthStencilState> depthStencilState = {}; 

    if (CheckDepthStencilFormat(parameters->DepthStencil.Format))
    {
        pipelineStateDescriptor->setDepthAttachmentPixelFormat(ConvertToMetalResourceFormat(parameters->DepthStencil.Format));

        auto depthStencilDescriptor = NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
   
        if (!parameters->DepthStencil.DepthDisableWrite)
        {
            depthStencilDescriptor->setDepthWriteEnabled(true);
        }

        depthStencilDescriptor->setDepthCompareFunction(ConvertToMetalCompareFunction(parameters->DepthStencil.DepthCompareFunction));
        depthStencilState = NS::TransferPtr(graphicsDeviceData->Device->newDepthStencilState(depthStencilDescriptor.get()));
    }

    auto cullMode = ConvertToMetalCullMode(parameters->CullMode);
    auto fillMode = parameters->FillMode == ElemGraphicsFillMode_Solid ? MTL::TriangleFillModeFill : MTL::TriangleFillModeLines;
    
    MetalShaderMetaData meshShaderMetaData = {};

    if (parameters->MeshShaderFunction)
    {
        auto functionData = GetMetalShaderFunction(shaderLibraryData, ShaderType_Mesh, parameters->MeshShaderFunction);

        if (!functionData.Function)
        {
            return ELEM_HANDLE_NULL;
        }

        pipelineStateDescriptor->setMeshFunction(functionData.Function.get());
        meshShaderMetaData.ThreadSizeX = functionData.MetaData.ThreadSizeX;
        meshShaderMetaData.ThreadSizeY = functionData.MetaData.ThreadSizeY;
        meshShaderMetaData.ThreadSizeZ = functionData.MetaData.ThreadSizeZ;
    }

    if (parameters->PixelShaderFunction)
    {
        auto functionData = GetMetalShaderFunction(shaderLibraryData, ShaderType_Pixel, parameters->PixelShaderFunction);

        if (!functionData.Function)
        {
            return ELEM_HANDLE_NULL;
        }

        pipelineStateDescriptor->setFragmentFunction(functionData.Function.get());
    }
    
    // TODO: Review this!
    auto dispatchGroup = dispatch_group_create();
    dispatch_group_enter(dispatchGroup);

    NS::SharedPtr<MTL::RenderPipelineState> pipelineState;
    NS::SharedPtr<MTL::RenderPipelineState>* pipelineStatePointer = &pipelineState;

    graphicsDeviceData->Device->newRenderPipelineState(pipelineStateDescriptor.get(), MTL::PipelineOptionNone, ^(MTL::RenderPipelineState* metalPipelineStatePointer, MTL::RenderPipelineReflection* reflectionPointer, NS::Error* errorPointer)
    {
        auto pipelineStateError = NS::TransferPtr(errorPointer);
        
        if (pipelineStateError->code() != 0)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Cannot compile pipeline state object. Error code: %ld: %s\n", pipelineStateError->code(), pipelineStateError->localizedFailureReason()->cString(NS::UTF8StringEncoding));
            return;
        }
    
        *pipelineStatePointer = NS::RetainPtr(metalPipelineStatePointer);
        dispatch_group_leave(dispatchGroup);
    });
    
    dispatch_group_wait(dispatchGroup, DISPATCH_TIME_FOREVER);
    
    auto handle = SystemAddDataPoolItem(metalPipelineStatePool, {
        .RenderPipelineState = pipelineState,
        .RenderDepthStencilState = depthStencilState,
        .RenderCullMode = cullMode,
        .RenderFillMode = fillMode,
        .MeshShaderMetaData = meshShaderMetaData
    }); 

    SystemAddDataPoolItemFull(metalPipelineStatePool, handle, {
    });

    return handle;
}

ElemPipelineState MetalCompileComputePipelineState(ElemGraphicsDevice graphicsDevice, const ElemComputePipelineStateParameters* parameters)
{
    InitMetalShaderLibraryMemory();
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    SystemAssert(parameters);
    SystemAssert(parameters->ShaderLibrary != ELEM_HANDLE_NULL);

    auto shaderLibraryData = GetMetalShaderLibraryData(parameters->ShaderLibrary);
    SystemAssert(shaderLibraryData);

    auto pipelineStateDescriptor = NS::TransferPtr(MTL::ComputePipelineDescriptor::alloc()->init());

    MetalShaderMetaData computeShaderMetaData = {};

    if (parameters->ComputeShaderFunction)
    {
        auto functionData = GetMetalShaderFunction(shaderLibraryData, ShaderType_Compute, parameters->ComputeShaderFunction);

        if (!functionData.Function)
        {
            return ELEM_HANDLE_NULL;
        }

        pipelineStateDescriptor->setComputeFunction(functionData.Function.get());
        computeShaderMetaData.ThreadSizeX = functionData.MetaData.ThreadSizeX;
        computeShaderMetaData.ThreadSizeY = functionData.MetaData.ThreadSizeY;
        computeShaderMetaData.ThreadSizeZ = functionData.MetaData.ThreadSizeZ;
    }

    // TODO: Review this!
    auto dispatchGroup = dispatch_group_create();
    dispatch_group_enter(dispatchGroup);

    NS::SharedPtr<MTL::ComputePipelineState> pipelineState;
    NS::SharedPtr<MTL::ComputePipelineState>* pipelineStatePointer = &pipelineState;

    graphicsDeviceData->Device->newComputePipelineState(pipelineStateDescriptor.get(), MTL::PipelineOptionNone, ^(MTL::ComputePipelineState* metalPipelineStatePointer, MTL::ComputePipelineReflection* reflectionPointer, NS::Error* errorPointer)
    {
        auto pipelineStateError = NS::TransferPtr(errorPointer);
        
        if (pipelineStateError->code() != 0)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Cannot compile pipeline state object. Error code: %ld: %s\n", pipelineStateError->code(), pipelineStateError->localizedFailureReason()->cString(NS::UTF8StringEncoding));
            return;
        }
    
        *pipelineStatePointer = NS::RetainPtr(metalPipelineStatePointer);
        dispatch_group_leave(dispatchGroup);
    });
    
    dispatch_group_wait(dispatchGroup, DISPATCH_TIME_FOREVER);
    
    auto handle = SystemAddDataPoolItem(metalPipelineStatePool, {
        .ComputePipelineState = pipelineState,
        .ComputeShaderMetaData = computeShaderMetaData
    }); 

    SystemAddDataPoolItemFull(metalPipelineStatePool, handle, {
    });

    return handle;
}

void MetalFreePipelineState(ElemPipelineState pipelineState)
{
}

void MetalBindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);
    SystemAssert(pipelineState != ELEM_HANDLE_NULL);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);
    
    auto graphicsDeviceData = GetMetalGraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto pipelineStateData = GetMetalPipelineStateData(pipelineState);
    SystemAssert(pipelineStateData);

    if (commandListData->CommandEncoderType == MetalCommandEncoderType_Render)
    {
        if (commandListData->PipelineState != pipelineState)
        {
            auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandListData->CommandEncoder.get();
            renderCommandEncoder->setRenderPipelineState(pipelineStateData->RenderPipelineState.get());

            if (pipelineStateData->RenderDepthStencilState)
            {
                renderCommandEncoder->setDepthStencilState(pipelineStateData->RenderDepthStencilState.get());
            }

            renderCommandEncoder->setCullMode(pipelineStateData->RenderCullMode);
            renderCommandEncoder->setTriangleFillMode(pipelineStateData->RenderFillMode);
        }
    }
    else
    {
        if (commandListData->CommandEncoderType == MetalCommandEncoderType_None)
        {
            auto computeCommandEncoder = NS::RetainPtr(commandListData->DeviceObject->computeCommandEncoder()); 
            commandListData->CommandEncoder = computeCommandEncoder;
            commandListData->CommandEncoderType = MetalCommandEncoderType_Compute;
            commandListData->PipelineState = ELEM_HANDLE_NULL;

            computeCommandEncoder->setBuffer(graphicsDeviceData->ResourceArgumentBuffer.Storage->ArgumentBuffer.get(), 0, 0);
            computeCommandEncoder->setBuffer(graphicsDeviceData->SamplerArgumentBuffer.Storage->ArgumentBuffer.get(), 0, 1);
        }
    
        if (commandListData->PipelineState != pipelineState)
        {
            auto computeCommandEncoder = (MTL::ComputeCommandEncoder*)commandListData->CommandEncoder.get();
            computeCommandEncoder->setComputePipelineState(pipelineStateData->ComputePipelineState.get());
        }
    }
        
    commandListData->PipelineState = pipelineState;
}

void MetalPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto pushData = SystemDuplicateBuffer<uint8_t>(stackMemoryArena, ReadOnlySpan(data.Items, data.Length)); 

    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);
    
    auto graphicsDeviceData = GetMetalGraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    if (commandListData->CommandEncoderType == MetalCommandEncoderType_Render)
    {
        auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandListData->CommandEncoder.get();

        // TODO: Do we need to check if the shader stage is bound?
        // TODO: compute offset
        // HACK: For the moment we set the slot 2 because it is the global one for bindless
        
        if (!commandListData->ArgumentBufferBound)
        {
            renderCommandEncoder->setMeshBuffer(graphicsDeviceData->ResourceArgumentBuffer.Storage->ArgumentBuffer.get(), 0, 0);
            renderCommandEncoder->setMeshBuffer(graphicsDeviceData->SamplerArgumentBuffer.Storage->ArgumentBuffer.get(), 0, 1);
            renderCommandEncoder->setFragmentBuffer(graphicsDeviceData->ResourceArgumentBuffer.Storage->ArgumentBuffer.get(), 0, 0);
            renderCommandEncoder->setFragmentBuffer(graphicsDeviceData->SamplerArgumentBuffer.Storage->ArgumentBuffer.get(), 0, 1);
            commandListData->ArgumentBufferBound = true;
        }

        renderCommandEncoder->setMeshBytes(pushData.Pointer, pushData.Length, 2);
        renderCommandEncoder->setFragmentBytes(pushData.Pointer, pushData.Length, 2);
    }
    else if (commandListData->CommandEncoderType == MetalCommandEncoderType_Compute)
    {
        // TODO: We need to bind the argument buffer too here
        auto computeCommandEncoder = (MTL::ComputeCommandEncoder*)commandListData->CommandEncoder.get();
        computeCommandEncoder->setBytes(pushData.Pointer, pushData.Length, 2);
    }
}

void MetalDispatchCompute(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

    if (!CheckMetalCommandEncoderType(commandListData, MetalCommandEncoderType_Compute))
    {
        return;
    }

    SystemAssert(commandListData->CommandEncoder);
    
    InsertMetalResourceBarriersIfNeeded(commandList, ElemGraphicsResourceBarrierSyncType_Compute);

    auto pipelineStateData = GetMetalPipelineStateData(commandListData->PipelineState);
    SystemAssert(pipelineStateData);

    auto computeCommandEncoder = (MTL::ComputeCommandEncoder*)commandListData->CommandEncoder.get();
    computeCommandEncoder->dispatchThreadgroups(MTL::Size(threadGroupCountX, threadGroupCountY, threadGroupCountZ), 
                                                MTL::Size(pipelineStateData->ComputeShaderMetaData.ThreadSizeX, pipelineStateData->ComputeShaderMetaData.ThreadSizeY, pipelineStateData->ComputeShaderMetaData.ThreadSizeZ));
}
