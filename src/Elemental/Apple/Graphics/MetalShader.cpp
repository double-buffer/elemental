#include "MetalShader.h"
#include "MetalGraphicsDevice.h"
#include "MetalCommandList.h"
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

ElemShaderLibrary MetalCreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData)
{
    InitMetalShaderLibraryMemory();
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);
    
    NS::SharedPtr<MTL::Library> metalLibrary = {};
    Span<NS::SharedPtr<MTL::Library>> graphicsShaderData = {};

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
        auto shaderCount = *(uint32_t*)dataSpan.Pointer;
        dataSpan = dataSpan.Slice(sizeof(uint32_t));

        // HACK: This is bad to allocate this here but this should be temporary
        graphicsShaderData = SystemPushArray<NS::SharedPtr<MTL::Library>>(MetalGraphicsMemoryArena, shaderCount);

        for (uint32_t i = 0; i < shaderCount; i++)
        {
            auto size = *(uint32_t*)dataSpan.Pointer;
            dataSpan = dataSpan.Slice(sizeof(uint32_t));

            auto dispatchData = dispatch_data_create(dataSpan.Pointer, size, nullptr, nullptr);

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

            dataSpan = dataSpan.Slice(size);
        }
    }

    auto handle = SystemAddDataPoolItem(metalShaderLibraryPool, {
        .MetalLibrary = metalLibrary,
        .GraphicsShaders = graphicsShaderData
    }); 

    return handle;
}

void MetalFreeShaderLibrary(ElemShaderLibrary shaderLibrary)
{
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

    // TODO: Ideally we should have only one mtl lib and get the function from it.
    // TODO: Amplification shader
    if (parameters->MeshShaderFunction)
    {
        for (uint32_t i = 0; i < shaderLibraryData->GraphicsShaders.Length; i++)
        {
            meshShaderFunction = NS::TransferPtr(shaderLibraryData->GraphicsShaders[i]->newFunction(NS::String::string(parameters->MeshShaderFunction, NS::UTF8StringEncoding)));

            if (meshShaderFunction)
            {
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
        
    pipelineStateDescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
    
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

    auto pipelineStateData = GetMetalPipelineStateData(pipelineState);
    SystemAssert(pipelineStateData);

    SystemAssert(commandListData->CommandEncoderType == MetalCommandEncoderType_Render);
    SystemAssert(commandListData->CommandEncoder);

    auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandListData->CommandEncoder.get();
    renderCommandEncoder->setRenderPipelineState(pipelineStateData->RenderPipelineState.get());
}

void MetalPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

    SystemAssert(commandListData->CommandEncoderType == MetalCommandEncoderType_Render);
    SystemAssert(commandListData->CommandEncoder);

    auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandListData->CommandEncoder.get();

    // TODO: Amplification shader
    // TODO: Do we need to check if the shader stage is bound?
    // TODO: compute offset
    // HACK: For the oment we set the slot 2 because it is the global one for bindless
    renderCommandEncoder->setMeshBytes(data.Items, data.Length, 2);
    renderCommandEncoder->setFragmentBytes(data.Items, data.Length, 2);
}
