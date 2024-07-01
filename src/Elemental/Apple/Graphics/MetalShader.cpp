#include "MetalShader.h"
#include "MetalGraphicsDevice.h"
#include "MetalCommandList.h"
#include "MetalResource.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

#define METAL_MAX_LIBRARIES UINT16_MAX
#define METAL_MAX_PIPELINESTATES UINT16_MAX

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
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Shader: %s", shaders[i].Name);

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

    auto shaderLibraryData= GetMetalShaderLibraryData(parameters->ShaderLibrary);
    SystemAssert(shaderLibraryData);

    auto pipelineStateDescriptor = NS::TransferPtr(MTL::MeshRenderPipelineDescriptor::alloc()->init());

    NS::SharedPtr<MTL::Function> meshShaderFunction;
    NS::SharedPtr<MTL::Function> pixelShaderFunction;

    uint32_t meshThreadGroupSizeX, meshThreadGroupSizeY, meshThreadGroupSizeZ = 0u;

    // TODO: Get the correct shader based on the function name
    if (parameters->MeshShaderFunction)
    {
        for (uint32_t i = 0; i < shaderLibraryData->GraphicsShaders.Length; i++)
        {
            meshShaderFunction = NS::TransferPtr(shaderLibraryData->GraphicsShaders[i]->newFunction(NS::String::string(parameters->MeshShaderFunction, NS::UTF8StringEncoding)));

            if (meshShaderFunction)
            {
                // TODO: Get correct metadata here it is just for test
                meshThreadGroupSizeX = shaderLibraryData->Shaders[i].Metadata[0].Value[0];
                meshThreadGroupSizeY = shaderLibraryData->Shaders[i].Metadata[0].Value[1];
                meshThreadGroupSizeZ = shaderLibraryData->Shaders[i].Metadata[0].Value[2];
                break;
            }
        }
            
        if (meshShaderFunction)
        {
            pipelineStateDescriptor->setMeshFunction(meshShaderFunction.get());
        }
        else
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Cannot find mesh shader function '%s'", parameters->MeshShaderFunction);
        }
    }

    if (parameters->PixelShaderFunction)
    {
        for (uint32_t i = 0; i < shaderLibraryData->GraphicsShaders.Length; i++)
        {
            pixelShaderFunction = NS::TransferPtr(shaderLibraryData->GraphicsShaders[i]->newFunction(NS::String::string(parameters->PixelShaderFunction, NS::UTF8StringEncoding)));

            if (pixelShaderFunction)
            {
                break;
            }

        }
            
        if (pixelShaderFunction)
        {
            pipelineStateDescriptor->setFragmentFunction(pixelShaderFunction.get());
        }
        else
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Cannot find pixel shader function '%s'", parameters->PixelShaderFunction);
        }
    }
        
    for (uint32_t i = 0; i < parameters->TextureFormats.Length; i++)
    {
        pipelineStateDescriptor->colorAttachments()->object(0)->setPixelFormat(ConvertToMetalResourceFormat(parameters->TextureFormats.Items[i]));
    }
    
    // TODO: Why is the triangle not back face culled by default?
    //pipelineStateDescriptor.supportIndirectCommandBuffers = true

    // TODO: Use the correct render target format
    /*if (renderPassDescriptor->RenderTarget0.HasValue) 
    {
        pipelineStateDescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
    }*/
    
    /*
    if (metalRenderPassDescriptor.RenderTarget2TextureFormat.HasValue == 1) {
        pipelineStateDescriptor.colorAttachments[1].pixelFormat = convertTextureFormat(metalRenderPassDescriptor.RenderTarget2TextureFormat.Value)
    }

    if (metalRenderPassDescriptor.RenderTarget3TextureFormat.HasValue == 1) {
        pipelineStateDescriptor.colorAttachments[2].pixelFormat = convertTextureFormat(metalRenderPassDescriptor.RenderTarget3TextureFormat.Value)
    }

    if (metalRenderPassDescriptor.RenderTarget4TextureFormat.HasValue == 1) {
        pipelineStateDescriptor.colorAttachments[3].pixelFormat = convertTextureFormat(metalRenderPassDescriptor.RenderTarget4TextureFormat.Value)
    }

    if (metalRenderPassDescriptor.DepthTexturePointer.HasValue == 1) {
        pipelineStateDescriptor.depthAttachmentPixelFormat = .depth32Float
    } 
)
    if (metalRenderPassDescriptor.RenderTarget1BlendOperation.HasValue == 1) {
        initBlendState(pipelineStateDescriptor.colorAttachments[0]!, metalRenderPassDescriptor.RenderTarget1BlendOperation.Value)
    }

    if (metalRenderPassDescriptor.RenderTarget2BlendOperation.HasValue == 1) {
        initBlendState(pipelineStateDescriptor.colorAttachments[1]!, metalRenderPassDescriptor.RenderTarget2BlendOperation.Value)
    }

    if (metalRenderPassDescriptor.RenderTarget3BlendOperation.HasValue == 1) {
        initBlendState(pipelineStateDescriptor.colorAttachments[2]!, metalRenderPassDescriptor.RenderTarget3BlendOperation.Value)
    }

    if (metalRenderPassDescriptor.RenderTarget4BlendOperation.HasValue == 1) {
        initBlendState(pipelineStateDescriptor.colorAttachments[3]!, metalRenderPassDescriptor.RenderTarget4BlendOperation.Value)
    }*/
    
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
        .ThreadSizeX = meshThreadGroupSizeX,
        .ThreadSizeY = meshThreadGroupSizeY,
        .ThreadSizeZ = meshThreadGroupSizeZ,
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

    NS::SharedPtr<MTL::Function> computeFunction;

    // TODO: Ideally we should have only one mtl lib and get the function from it.
    // TODO: Amplification shader
    // TODO: Change that
    if (parameters->ComputeShaderFunction)
    {
        for (uint32_t i = 0; i < shaderLibraryData->GraphicsShaders.Length; i++)
        {
            computeFunction = NS::TransferPtr(shaderLibraryData->GraphicsShaders[i]->newFunction(NS::String::string(parameters->ComputeShaderFunction, NS::UTF8StringEncoding)));

            if (computeFunction)
            {
                break;
            }

        }
            
        if (computeFunction)
        {
            pipelineStateDescriptor->setComputeFunction(computeFunction.get());
        }
        else
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Cannot find compute shader function '%s'", parameters->ComputeShaderFunction);
            return ELEM_HANDLE_NULL;
        }
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
        // TODO: Get correct metadata here it is just for test
        .ThreadSizeX = shaderLibraryData->Shaders[0].Metadata[0].Value[0],
        .ThreadSizeY = shaderLibraryData->Shaders[0].Metadata[0].Value[1],
        .ThreadSizeZ = shaderLibraryData->Shaders[0].Metadata[0].Value[2],
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
        auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandListData->CommandEncoder.get();
        renderCommandEncoder->setRenderPipelineState(pipelineStateData->RenderPipelineState.get());
    }
    else
    {
        if (commandListData->CommandEncoderType == MetalCommandEncoderType_None)
        {
            auto computeCommandEncoder = NS::RetainPtr(commandListData->DeviceObject->computeCommandEncoder()); 
            commandListData->CommandEncoder = computeCommandEncoder;
            commandListData->CommandEncoderType = MetalCommandEncoderType_Compute;
        }
    
        auto computeCommandEncoder = (MTL::ComputeCommandEncoder*)commandListData->CommandEncoder.get();
        computeCommandEncoder->setComputePipelineState(pipelineStateData->ComputePipelineState.get());
    }
        
    commandListData->PipelineState = pipelineState;
}

void MetalPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);
    
    auto graphicsDeviceData = GetMetalGraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    if (commandListData->CommandEncoderType == MetalCommandEncoderType_Render)
    {
        auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandListData->CommandEncoder.get();

        // TODO: Amplification shader
        // TODO: Do we need to check if the shader stage is bound?
        // TODO: compute offset
        // HACK: For the oment we set the slot 2 because it is the global one for bindless
        
        renderCommandEncoder->setMeshBuffer(graphicsDeviceData->ResourceArgumentBuffer.Storage->ArgumentBuffer.get(), 0, 0);
        renderCommandEncoder->setMeshBytes(data.Items, data.Length, 2);

        renderCommandEncoder->setFragmentBuffer(graphicsDeviceData->ResourceArgumentBuffer.Storage->ArgumentBuffer.get(), 0, 0);
        renderCommandEncoder->setFragmentBytes(data.Items, data.Length, 2);
    }
    else if (commandListData->CommandEncoderType == MetalCommandEncoderType_Compute)
    {
        auto computeCommandEncoder = (MTL::ComputeCommandEncoder*)commandListData->CommandEncoder.get();

        // TODO: Amplification shader
        // TODO: Do we need to check if the shader stage is bound?
        // TODO: compute offset
        // HACK: For the oment we set the slot 2 because it is the global one for bindless
        computeCommandEncoder->setBuffer(graphicsDeviceData->ResourceArgumentBuffer.Storage->ArgumentBuffer.get(), 0, 0);
        computeCommandEncoder->setBytes(data.Items, data.Length, 2);
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

    auto pipelineStateData = GetMetalPipelineStateData(commandListData->PipelineState);
    SystemAssert(pipelineStateData);

    auto computeCommandEncoder = (MTL::ComputeCommandEncoder*)commandListData->CommandEncoder.get();
    computeCommandEncoder->dispatchThreadgroups(MTL::Size(threadGroupCountX, threadGroupCountY, threadGroupCountZ), 
                                                MTL::Size(pipelineStateData->ThreadSizeX, pipelineStateData->ThreadSizeY, pipelineStateData->ThreadSizeZ));
}
