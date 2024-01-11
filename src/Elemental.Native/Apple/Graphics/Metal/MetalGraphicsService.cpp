#include "MetalGraphicsService.h"
#include "MetalGraphicsDevice.h"
#include "MetalCommandQueue.h"
#include "MetalCommandList.h"
#include "MetalSwapChain.h"
#include "MetalTexture.h"

#include "SystemMemory.h"
#include "SystemLogging.h"

MemoryArena metalMemoryArena;

static NS::SharedPtr<MTL::SharedEventListener> _sharedEventListener;

DllExport void Native_InitGraphicsService(GraphicsServiceOptions* options)
{
    _sharedEventListener = NS::TransferPtr(MTL::SharedEventListener::alloc()->init());
    metalMemoryArena = SystemAllocateMemoryArena();
}

DllExport void Native_FreeGraphicsService()
{
    _sharedEventListener.reset();
}

DllExport void Native_GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count)
{
    (*count) = 0;
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto devices = NS::RetainPtr(MTLCopyAllDevices());
    auto finalDevices = new NS::SharedPtr<MTL::Device>[64];
    
    for (uint32_t i = 0; i < devices->count(); i++)
    {
        finalDevices[i] = NS::RetainPtr((MTL::Device*)devices->object(i));
    }

    for (uint32_t i = 0; i < devices->count(); i++)
    {
        for (uint32_t j = i + 1; j < devices->count(); j++)
        {
            auto device1 = finalDevices[i];
            auto device2 = finalDevices[j];

            auto device1IsBetter = false;

            // TODO: Review that
            if (device1->location() == MTL::DeviceLocationExternal && device2->location() != MTL::DeviceLocationExternal)
            {
                device1IsBetter = true; // device1 is external, device2 is not
            } 
            else if (device1->location() != MTL::DeviceLocationExternal && device2->location() == MTL::DeviceLocationExternal) 
            {
                device1IsBetter = false; // device2 is external, device1 is not
            } 
            else if (device1->location() == MTL::DeviceLocationSlot && device2->location() != MTL::DeviceLocationSlot)
            {
                device1IsBetter = true; // device1 is slot, device2 is not
            }
            else if (device1->location() != MTL::DeviceLocationSlot && device2->location() == MTL::DeviceLocationSlot)
            {
                device1IsBetter = false; // device2 is slot, device1 is not
            } 
            else
            {
                device1IsBetter = device1->recommendedMaxWorkingSetSize() > device2->recommendedMaxWorkingSetSize();
            }

            if (!device1IsBetter)
            {
                auto temp = finalDevices[i];
                finalDevices[i] = finalDevices[j];
                finalDevices[j] = temp;
            }
        }
    }
    
    for (uint32_t i = 0; i < devices->count(); i++)
    {
        auto device = finalDevices[i];
        graphicsDevices[(*count)++] = ConstructGraphicsDeviceInfo(device);
    }
}

DllExport void* Native_CreateGraphicsDevice(GraphicsDeviceOptions* options)
{
    auto devices = NS::TransferPtr(MTLCopyAllDevices());
    NS::SharedPtr<MTL::Device> metalDevice;

    for (uint32_t i = 0; i < devices->count(); i++)
    {
        auto device = NS::RetainPtr((MTL::Device*)devices->object(i));

        if (device->registryID() == options->DeviceId)
        {
            metalDevice = device;
            break;
        }
    }

    if (!metalDevice.get())
    {
        return nullptr;
    }

    auto graphicsDevice = new MetalGraphicsDevice();
    graphicsDevice->MetalDevice = metalDevice; 
    graphicsDevice->PipelineStates = SystemCreateDictionary<uint64_t, MetalPipelineStateCacheItem>(metalMemoryArena, 1024);
    return graphicsDevice;
}

DllExport void Native_FreeGraphicsDevice(void* graphicsDevicePointer)
{
    auto graphicsDevice = (MetalGraphicsDevice*)graphicsDevicePointer;
    auto cacheEnumerator = SystemGetDictionaryEnumerator(graphicsDevice->PipelineStates);
    auto cacheEnumItem = SystemGetDictionaryEnumeratorNextValue(&cacheEnumerator);
    
    while (cacheEnumItem != nullptr)
    {
        cacheEnumItem->PipelineState.reset();
        cacheEnumItem = SystemGetDictionaryEnumeratorNextValue(&cacheEnumerator);
    }

    delete graphicsDevice;
}

DllExport GraphicsDeviceInfo Native_GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    auto graphicsDevice = (MetalGraphicsDevice*)graphicsDevicePointer;
    return ConstructGraphicsDeviceInfo(graphicsDevice->MetalDevice);
}

DllExport void* Native_CreateCommandQueue(void* graphicsDevicePointer, CommandQueueType type)
{
    auto graphicsDevice = (MetalGraphicsDevice*)graphicsDevicePointer;
    auto metalCommandQueue = NS::TransferPtr(graphicsDevice->MetalDevice->newCommandQueue(64));

    if (!metalCommandQueue.get())
    {
        return nullptr;
    } 

    // TODO: If CPU sync is not needed we can use something faster
    auto metalSharedEvent = NS::TransferPtr(graphicsDevice->MetalDevice->newSharedEvent());

    auto commandQueue = new MetalCommandQueue();
    commandQueue->GraphicsDevice = graphicsDevice;
    commandQueue->FenceValue = 0;
    commandQueue->DeviceObject = metalCommandQueue;
    commandQueue->Fence = metalSharedEvent;

    return commandQueue;
}

DllExport void Native_FreeCommandQueue(void* commandQueuePointer)
{
    auto commandQueue = (MetalCommandQueue*)commandQueuePointer;
    delete commandQueue;
}

DllExport void Native_SetCommandQueueLabel(void* commandQueuePointer, uint8_t* label)
{
    auto commandQueue = (MetalCommandQueue*)commandQueuePointer;
    commandQueue->DeviceObject->setLabel(NS::String::string((const char*)label, NS::UTF8StringEncoding));
}

DllExport void* Native_CreateCommandList(void* commandQueuePointer)
{
    // TODO: Create an autoreleasePool

    auto commandQueue = (MetalCommandQueue*)commandQueuePointer;
    
    // TODO: Check if we need transfer or retain
    auto metalCommandBuffer = NS::RetainPtr(commandQueue->DeviceObject->commandBufferWithUnretainedReferences());

    if (!metalCommandBuffer.get())
    {
        return nullptr;
    }

    auto commandList = new MetalCommandList();
    commandList->GraphicsDevice = commandQueue->GraphicsDevice;
    commandList->IsRenderPassActive = 0;
    commandList->DeviceObject = metalCommandBuffer;

    return commandList;
}

DllExport void Native_FreeCommandList(void* commandListPointer)
{
    // TODO: Release autoreleasePool if not released
    auto commandList = (MetalCommandList*)commandListPointer;
    delete commandList;
}

DllExport void Native_SetCommandListLabel(void* commandListPointer, uint8_t* label)
{
    auto commandList = (MetalCommandList*)commandListPointer;
    commandList->DeviceObject->setLabel(NS::String::string((const char*)label, NS::UTF8StringEncoding));
}

DllExport void Native_CommitCommandList(void* commandListPointer)
{
    // TODO: Release autoreleasePool
    auto commandList = (MetalCommandList*)commandListPointer;
    
    if (commandList->CommandEncoder.get())
    {
        commandList->CommandEncoder->endEncoding();
        commandList->CommandEncoder.reset();
    }
}
    
DllExport Fence Native_ExecuteCommandLists(void* commandQueuePointer, void** commandLists, int32_t commandListCount, Fence* fencesToWait, int32_t fenceToWaitCount)
{
    auto commandQueue = (MetalCommandQueue*)commandQueuePointer;

    if (fenceToWaitCount > 0)
    {
        // TODO: Check if we need transfer or retain
        auto waitCommandBuffer = NS::RetainPtr(commandQueue->DeviceObject->commandBufferWithUnretainedReferences());
        
        if (!waitCommandBuffer.get())
        {
            printf("Error: executeCommandLists: Error while creating wait command buffer object.\n");
            return Fence();
        }

        waitCommandBuffer->setLabel(MTLSTR("WaitCommandBuffer"));

        for (uint32_t i = 0; i < fenceToWaitCount; i++)
        {
            auto fenceToWait = fencesToWait[i];
            auto commandQueueToWait = (MetalCommandQueue*)fenceToWait.CommandQueuePointer;

            waitCommandBuffer->encodeWait(commandQueueToWait->Fence.get(), fenceToWait.FenceValue);
        }

        waitCommandBuffer->commit();
    }

    uint64_t fenceValue = 0;

    for (uint32_t i = 0; i < commandListCount; i++)
    {
        auto commandList = (MetalCommandList*)commandLists[i];

        if (i == commandListCount - 1)
        {
            fenceValue = commandQueue->FenceValue.fetch_add(1) + 1;
            commandList->DeviceObject->encodeSignalEvent(commandQueue->Fence.get(), fenceValue);
        }

        commandList->DeviceObject->commit();
        commandList->CommandEncoder.reset(); 
    }

    Fence fence = {};
    fence.CommandQueuePointer = commandQueuePointer;
    fence.FenceValue = fenceValue;

    return fence;
}

DllExport void Native_WaitForFenceOnCpu(Fence fence)
{
    auto commandQueueToWait = (MetalCommandQueue*)fence.CommandQueuePointer;

    if (commandQueueToWait->Fence->signaledValue() < fence.FenceValue)
    {
        auto dispatchGroup = dispatch_group_create();
        dispatch_group_enter(dispatchGroup);

        commandQueueToWait->Fence->notifyListener(_sharedEventListener.get(), fence.FenceValue, ^(MTL::SharedEvent* pEvent, uint64_t value)
        {
            dispatch_group_leave(dispatchGroup);
        });

        dispatch_group_wait(dispatchGroup, DISPATCH_TIME_FOREVER);
    }
}
    
DllExport void Native_ResetCommandAllocation(void* graphicsDevicePointer)
{
}

DllExport void Native_FreeTexture(void* texturePointer)
{
    auto texture = (MetalTexture*)texturePointer;
    delete texture;
}

DllExport void* Native_CreateSwapChain(void* windowPointer, void* commandQueuePointer, SwapChainOptions* options)
{
    auto commandQueue = (MetalCommandQueue*)commandQueuePointer;
    auto graphicsDevice = commandQueue->GraphicsDevice;
    
    // TODO: Refactor: this is MacOS specific code!
    auto window = (MacOSWindow*)windowPointer;
    auto contentView = window->WindowHandle->contentView();

    auto metalLayer = NS::TransferPtr(CA::MetalLayer::layer());
    contentView->setLayer(metalLayer.get());
    
    auto renderSize = Native_GetWindowRenderSize((MacOSWindow*)windowPointer);

    if (options->Width != 0) 
    {
        renderSize.Width = options->Width;
    }
    
    if (options->Height != 0) 
    {
        renderSize.Height = options->Height;
    }

    metalLayer->setDevice(graphicsDevice->MetalDevice.get());
    metalLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
    metalLayer->setFramebufferOnly(true);
    metalLayer->setDrawableSize(CGSizeMake(renderSize.Width, renderSize.Height));

    auto swapChain = new MetalSwapChain();
    swapChain->GraphicsDevice = graphicsDevice;
    swapChain->DeviceObject = metalLayer;
    swapChain->CommandQueue = commandQueue;
    swapChain->PresentSemaphore = dispatch_semaphore_create(options->MaximumFrameLatency);

    return swapChain;
}

DllExport void Native_FreeSwapChain(void* swapChainPointer)
{
    auto swapChain = (MetalSwapChain*)swapChainPointer;
    free(swapChain->PresentSemaphore);
    delete swapChain;
}
    
DllExport void Native_ResizeSwapChain(void* swapChainPointer, int width, int height)
{
    auto swapChain = (MetalSwapChain*)swapChainPointer;
    swapChain->DeviceObject->setDrawableSize(CGSizeMake(width, height));
}
    
DllExport void* Native_GetSwapChainBackBufferTexture(void* swapChainPointer)
{
    auto swapChain = (MetalSwapChain*)swapChainPointer;

    if (!swapChain->BackBufferDrawable.get())
    {
        printf("Warning: Backbuffer drawable is null.\n");
        return nullptr;
    }

    auto texture = new MetalTexture();
    texture->GraphicsDevice = swapChain->GraphicsDevice;
    texture->DeviceObject = NS::RetainPtr(swapChain->BackBufferDrawable->texture());
    texture->IsPresentTexture = true;
    
    return texture;
}

DllExport void Native_PresentSwapChain(void* swapChainPointer)
{
    auto swapChain = (MetalSwapChain*)swapChainPointer;

    // HACK: Can we avoid creating an empty command buffer?
    auto commandBuffer = NS::RetainPtr(swapChain->CommandQueue->DeviceObject->commandBufferWithUnretainedReferences());

    if (!commandBuffer.get())
    {
        printf("presentSwapChain: Error while creating command buffer object.\n");
        return;
    }
    
    if (!swapChain->BackBufferDrawable.get())
    {
        printf("Warning: PresentSwapChain: Backbuffer drawable is null.\n");
        return;
    }
    
    auto drawable = swapChain->BackBufferDrawable;
   
    commandBuffer->addScheduledHandler([drawable](MTL::CommandBuffer* commandBuffer)
    {
        drawable->present();
    });
    
    commandBuffer->addCompletedHandler([swapChain](MTL::CommandBuffer* commandBuffer)
    {
        dispatch_semaphore_signal(swapChain->PresentSemaphore);
    }); 

    commandBuffer->setLabel(MTLSTR("PresentSwapChainCommandBuffer"));
    commandBuffer->commit();
}

DllExport void Native_WaitForSwapChainOnCpu(void* swapChainPointer)
{
    auto swapChain = (MetalSwapChain*)swapChainPointer;

    dispatch_semaphore_wait(swapChain->PresentSemaphore, DISPATCH_TIME_FOREVER);
    swapChain->BackBufferDrawable.reset();

    auto nextMetalDrawable = NS::RetainPtr(swapChain->DeviceObject->nextDrawable());
        
    if (!nextMetalDrawable.get())
    {
        printf("WARNING: waitForSwapChainOnCpu: Cannot acquire a back buffer");
        return;
    }

    swapChain->BackBufferDrawable = nextMetalDrawable;
}

DllExport void* Native_CreateShader(void* graphicsDevicePointer, ShaderPart* shaderParts, int32_t shaderPartCount)
{
    auto graphicsDevice = (MetalGraphicsDevice*)graphicsDevicePointer;
    
    auto shader = new MetalShader();
    shader->GraphicsDevice = graphicsDevice;

    for (uint32_t i = 0; i < shaderPartCount; i++)
    {
        auto shaderPart = shaderParts[i];

        auto dispatchData = dispatch_data_create(shaderPart.DataPointer, shaderPart.DataCount, nullptr, nullptr);

        NS::Error* errorPointer;
        auto metalLibrary = NS::TransferPtr(graphicsDevice->MetalDevice->newLibrary(dispatchData, &errorPointer));
        auto libraryError = NS::TransferPtr(errorPointer);

        if (libraryError->code() != 0)
        {
            printf("Error: CreateShader: Error code: %ld: %s\n", libraryError->code(), libraryError->localizedFailureReason()->cString(NS::UTF8StringEncoding));
            return nullptr;
        }

        auto shaderFunction = NS::TransferPtr(metalLibrary->newFunction(NS::String::string((char*)shaderPart.EntryPoint, NS::UTF8StringEncoding)));

        if (!shaderFunction.get())
        {
            printf("Error: CreateShader: Shader function not found!\n");
            return nullptr;
        }

        uint32_t threadCountX = 0;
        uint32_t threadCountY = 0;
        uint32_t threadCountZ = 0;

        for (uint32_t j = 0; j < shaderPart.MetaDataCount; j++)
        {
            auto metaData = shaderPart.MetaDataPointer[j];

            switch (metaData.Type)
            {
                case ShaderMetaDataType_ThreadCountX:
                    threadCountX = metaData.Value;
                    break;

                case ShaderMetaDataType_ThreadCountY:
                    threadCountY = metaData.Value;
                    break;
                    
                case ShaderMetaDataType_ThreadCountZ:
                    threadCountZ = metaData.Value;
                    break;

                default:
                    break;
            }

            auto threadCount = MTL::Size(threadCountX, threadCountY, threadCountZ);

            switch (shaderPart.Stage)
            {
                case ShaderStage_AmplificationShader:
                    shader->AmplificationShader = shaderFunction;
                    shader->AmplificationThreadCount = threadCount;
                    break;
                    
                case ShaderStage_MeshShader:
                    shader->MeshShader = shaderFunction;
                    shader->MeshThreadCount = threadCount;
                    break;

                case ShaderStage_PixelShader:
                    shader->PixelShader = shaderFunction;
                    break;

                default:
                    printf("ERROR: CreateShader: Unknown shader part.\n");
                    break;
            }
        }
    }

    return shader;
}

DllExport void Native_FreeShader(void* shaderPointer)
{
    auto shader = (MetalShader*)shaderPointer;
    delete shader;
}

DllExport void Native_BeginRenderPass(void* commandListPointer, RenderPassDescriptor* renderPassDescriptor)
{
    if (renderPassDescriptor == nullptr)
    {
        // TODO: Log warning
        return;
    }

    auto commandList = (MetalCommandList*)commandListPointer;

    // TODO: Add an util function that checks the current encoder and create a new one if it can.
    // It will also check if the command queue is compatible
    if (commandList->CommandEncoder.get())
    {
        commandList->CommandEncoder->endEncoding();
        commandList->CommandEncoder.reset();
    }

    commandList->IsRenderPassActive = true;
    commandList->CurrentRenderPassDescriptor = *renderPassDescriptor;
    
    commandList->CurrentMetalRenderPassDescriptor = NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());

    if (renderPassDescriptor->RenderTarget0.HasValue)
    {
        InitRenderPassDescriptor(commandList->CurrentMetalRenderPassDescriptor->colorAttachments()->object(0), &renderPassDescriptor->RenderTarget0.Value);
    }
    
    if (renderPassDescriptor->RenderTarget1.HasValue)
    {
        InitRenderPassDescriptor(commandList->CurrentMetalRenderPassDescriptor->colorAttachments()->object(1), &renderPassDescriptor->RenderTarget1.Value);
    }

    if (renderPassDescriptor->RenderTarget2.HasValue)
    {
        InitRenderPassDescriptor(commandList->CurrentMetalRenderPassDescriptor->colorAttachments()->object(2), &renderPassDescriptor->RenderTarget2.Value);
    }
    
    if (renderPassDescriptor->RenderTarget3.HasValue)
    {
        InitRenderPassDescriptor(commandList->CurrentMetalRenderPassDescriptor->colorAttachments()->object(3), &renderPassDescriptor->RenderTarget3.Value);
    }

    auto renderCommandEncoder = NS::RetainPtr(commandList->DeviceObject->renderCommandEncoder(commandList->CurrentMetalRenderPassDescriptor.get()));

    if (!renderCommandEncoder.get())
    {
        printf("ERROR: beginRenderPass: Render command encoder creation failed.\n");
        return;
    }

    commandList->CommandEncoder = renderCommandEncoder;

    // TODO: Depth buffer
    
    /*
        if (renderPassDescriptor.DepthBufferOperation == Write || renderPassDescriptor.DepthBufferOperation == ClearWrite) {
            renderCommandEncoder.setDepthStencilState(self.depthWriteOperationState)
        } else if (renderPassDescriptor.DepthBufferOperation == CompareEqual) {
            renderCommandEncoder.setDepthStencilState(self.depthCompareEqualState)
        } else if (renderPassDescriptor.DepthBufferOperation == CompareGreater) {
            renderCommandEncoder.setDepthStencilState(self.depthCompareGreaterState)
        } else {
            renderCommandEncoder.setDepthStencilState(self.depthNoneOperationState)
        }

        if (renderPassDescriptor.BackfaceCulling == 1) {
            renderCommandEncoder.setCullMode(.back)
        } else {
            renderCommandEncoder.setCullMode(.none)
        }*/

        // TODO: Use an actual viewport object and if null do that
        if (commandList->CurrentMetalRenderPassDescriptor->colorAttachments()->object(0)->texture() != nullptr) 
        {
            auto texture = commandList->CurrentMetalRenderPassDescriptor->colorAttachments()->object(0)->texture();
            renderCommandEncoder->setViewport({ .originX = 0.0, .originY = 0.0, .width = (double)texture->width(), .height = (double)texture->height(), .znear = 0.0, .zfar = 1.0 });
            renderCommandEncoder->setScissorRect({ .x = 0, .y = 0, .width = texture->width(), .height = texture->height() });
        }
        /* else if (metalRenderPassDescriptor.depthAttachment.texture != nil) {
            renderCommandEncoder.setViewport(MTLViewport(originX: 0.0, originY: 0.0, width: Double(metalRenderPassDescriptor.depthAttachment.texture!.width), height: Double(metalRenderPassDescriptor.depthAttachment.texture!.height), znear: 0.0, zfar: 1.0))
            renderCommandEncoder.setScissorRect(MTLScissorRect(x: 0, y: 0, width: metalRenderPassDescriptor.depthAttachment.texture!.width, height: metalRenderPassDescriptor.depthAttachment.texture!.height))
        }*/
}
    
DllExport void Native_EndRenderPass(void* commandListPointer)
{
    auto commandList = (MetalCommandList*)commandListPointer;
    
    if (!commandList->CommandEncoder.get())
    {
        printf("Error: endRenderPass: Command encoder is null\n");
        return;
    }

    commandList->CommandEncoder->endEncoding();
    commandList->CommandEncoder.reset();
    commandList->IsRenderPassActive = false;
    commandList->CurrentRenderPassDescriptor = {};
}

DllExport void Native_SetShader(void* commandListPointer, void* shaderPointer)
{
    auto commandList = (MetalCommandList*)commandListPointer;
    auto graphicsDevice = commandList->GraphicsDevice;
    auto shader = (MetalShader*)shaderPointer;
    
    if (commandList->IsRenderPassActive)
    {
        // TODO: Hash the parameters
        // TODO: Async compilation with mutlithread support. (Reserve a slot in the cache, and return the pipelinestate cache object)
        // TODO: Have a separate CompileShader function that will launch the async work.
        // TODO: Have a separate GetShaderStatus method
        // TODO: Block for this method, because it means the user wants to use the shader and wants to wait on purpose

        // TODO: This method is not thread-safe!
        auto hash = ComputeRenderPipelineStateHash(shader, &commandList->CurrentRenderPassDescriptor);

        if (!SystemDictionaryContainsKey(graphicsDevice->PipelineStates, hash))
        {
            printf("Create PipelineState for shader %llu...\n", hash);
            auto pipelineState = CreateRenderPipelineState(shader, &commandList->CurrentRenderPassDescriptor);
            SystemAddDictionaryEntry(graphicsDevice->PipelineStates, hash, pipelineState);
        }

        auto pipelineState = graphicsDevice->PipelineStates[hash].PipelineState;
        auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandList->CommandEncoder.get();
        renderCommandEncoder->setRenderPipelineState(pipelineState.get());

        commandList->CurrentRenderPipelineState = pipelineState;
        commandList->CurrentShader = shader;
    }
}
    
DllExport void Native_SetShaderConstants(void* commandListPointer, uint32_t slot, void* constantValues, int32_t constantValueCount)
{
    auto commandList = (MetalCommandList*)commandListPointer;
    
    if (commandList->IsRenderPassActive)
    {
        auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandList->CommandEncoder.get();
    
        // TODO: Make a common functions for this
        if (!commandList->CommandEncoder.get())
        {
            printf("Error: dispatchMesh: Command encoder is not a render command encoder\n");
            return;
        }

        if (!commandList->CurrentShader)
        {
            printf("error: dispatchMesh: No shader bound.\n");
            return;
        }

        if (commandList->CurrentShader->AmplificationShader.get())
        {
            renderCommandEncoder->setObjectBytes(constantValues, constantValueCount, slot);
        } 

        if (commandList->CurrentShader->MeshShader.get())
        {
            renderCommandEncoder->setMeshBytes(constantValues, constantValueCount, slot);
        } 
        
        if (commandList->CurrentShader->PixelShader.get())
        {
            renderCommandEncoder->setFragmentBytes(constantValues, constantValueCount, slot);
        } 
    }
}
    
DllExport void Native_DispatchMesh(void* commandListPointer, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    auto commandList = (MetalCommandList*)commandListPointer;

    if (!commandList->CommandEncoder.get())
    {
        printf("Error: dispatchMesh: Command encoder is not a render command encoder\n");
        return;
    }

    if (!commandList->CurrentShader)
    {
        printf("error: dispatchMesh: No shader bound.\n");
        return;
    }

    assert(commandList->CurrentShader->AmplificationShader.get() == nullptr 
       || (commandList->CurrentShader->AmplificationThreadCount.width > 0 
        && commandList->CurrentShader->AmplificationThreadCount.height > 0 
        && commandList->CurrentShader->AmplificationThreadCount.depth > 0));
        
    assert(commandList->CurrentShader->MeshShader.get() == nullptr 
       || (commandList->CurrentShader->MeshThreadCount.width > 0 
        && commandList->CurrentShader->MeshThreadCount.height > 0 
        && commandList->CurrentShader->MeshThreadCount.depth > 0));

    auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandList->CommandEncoder.get();

    renderCommandEncoder->drawMeshThreadgroups(MTL::Size(threadGroupCountX, threadGroupCountY, threadGroupCountZ), 
        commandList->CurrentShader->AmplificationShader.get() ? commandList->CurrentShader->AmplificationThreadCount : MTL::Size(32, 1, 1), 
        commandList->CurrentShader->MeshShader.get() ? commandList->CurrentShader->MeshThreadCount : MTL::Size(32, 1, 1));
}
    
GraphicsDeviceInfo ConstructGraphicsDeviceInfo(NS::SharedPtr<MTL::Device> device)
{
    GraphicsDeviceInfo result = {};
    result.DeviceId = device->registryID();
    result.DeviceName = device->name()->cString(NS::UTF8StringEncoding);
    result.GraphicsApi = GraphicsApi_Metal;
    result.AvailableMemory = device->recommendedMaxWorkingSetSize();

    return result;
}

void InitRenderPassDescriptor(MTL::RenderPassColorAttachmentDescriptor* metalDescriptor, RenderPassRenderTarget* renderTargetDescriptor)
{
    auto texture = (MetalTexture*)renderTargetDescriptor->TexturePointer;
    metalDescriptor->setTexture(texture->DeviceObject.get());

    if (texture->IsPresentTexture)
    {
        metalDescriptor->setLoadAction(MTL::LoadActionDontCare);
        metalDescriptor->setStoreAction(MTL::StoreActionStore); 
    }
    else
    {
        /*let resourceFence = self.graphicsDevice.makeFence()!
            colorTexture.resourceFence = resourceFence

            commandList.resourceFences.append(resourceFence)

            metalRenderPassDescriptor.colorAttachments[0].storeAction = .store*/
    }
    
    // TODO: Texture transitions with metal fences

    if (renderTargetDescriptor->ClearColor.HasValue) 
    {
        auto clearColor = renderTargetDescriptor->ClearColor.Value;

        metalDescriptor->setLoadAction(MTL::LoadActionClear);
        metalDescriptor->setClearColor(MTL::ClearColor(clearColor.X, clearColor.Y, clearColor.Z, clearColor.W));
    }

/*


        if (texture.isPresentTexture) {
            descriptor.loadAction = .dontCare
            descriptor.storeAction = .store
        } else {
            let resourceFence = self.graphicsDevice.makeFence()!
            colorTexture.resourceFence = resourceFence

            commandList.resourceFences.append(resourceFence)

            metalRenderPassDescriptor.colorAttachments[0].storeAction = .store
        }

        // TODO: Texture transitions with metal fences
        
        if (renderTargetDescriptor.ClearColor.HasValue) {
            let clearColor = renderTargetDescriptor.ClearColor.Value

            descriptor.loadAction = .clear
            descriptor.clearColor = MTLClearColor.init(red: Double(clearColor.X), green: Double(clearColor.Y), blue: Double(clearColor.Z), alpha: Double(clearColor.W))
        }*/
}

uint64_t ComputeRenderPipelineStateHash(MetalShader* shader, RenderPassDescriptor* renderPassDescriptor)
{
    // TODO: For the moment the hash of the shader is base on the pointer
    // Maybe we should base it on the hash of each shader parts data? 
    // This would prevent creating duplicate PSO if 2 shaders contains the same parts (it looks like an edge case)
    // but this would add more processing to generate the hash and this function is perf critical

    // TODO: Hash other render pass parameters
    // TODO: Use FarmHash64? https://github.com/TommasoBelluzzo/FastHashes/tree/master

    return (uint64_t)shader;
}

MetalPipelineStateCacheItem CreateRenderPipelineState(MetalShader* shader, RenderPassDescriptor* renderPassDescriptor)
{
    auto pipelineStateDescriptor = NS::TransferPtr(MTL::MeshRenderPipelineDescriptor::alloc()->init());

    if (shader->AmplificationShader.get())
    {
        pipelineStateDescriptor->setObjectFunction(shader->AmplificationShader.get());
    }

    if (shader->MeshShader.get())
    {
        pipelineStateDescriptor->setMeshFunction(shader->MeshShader.get());
    }

    if (shader->PixelShader.get())
    {
        pipelineStateDescriptor->setFragmentFunction(shader->PixelShader.get());
    }
    
    // TODO: Why is the triangle not back face culled by default?
    //pipelineStateDescriptor.supportIndirectCommandBuffers = true

    // TODO: Use the correct render target format
    if (renderPassDescriptor->RenderTarget0.HasValue) 
    {
        pipelineStateDescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
    }
    
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

    auto result = MetalPipelineStateCacheItem();
    auto tmp = &result;

    shader->GraphicsDevice->MetalDevice->newRenderPipelineState(pipelineStateDescriptor.get(), MTL::PipelineOptionNone, ^(MTL::RenderPipelineState* metalPipelineStatePointer, MTL::RenderPipelineReflection* reflectionPointer, NS::Error* errorPointer)
    {
        auto pipelineStateError = NS::TransferPtr(errorPointer);
        
        if (pipelineStateError->code() != 0)
        {
            printf("Error: CreateRenderPipelineState: Error code: %ld: %s\n", pipelineStateError->code(), pipelineStateError->localizedFailureReason()->cString(NS::UTF8StringEncoding));
            return;
        }
    
        tmp->PipelineState = NS::RetainPtr(metalPipelineStatePointer);
        dispatch_group_leave(dispatchGroup);
    });

    dispatch_group_wait(dispatchGroup, DISPATCH_TIME_FOREVER);
        
    return result;
}
