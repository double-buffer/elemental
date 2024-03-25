#include "MetalSwapChain.h"
#include "MetalCommandList.h"
#include "MetalGraphicsDevice.h"
#include "MetalTexture.h"
#include "../MacOSWindow.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"

#define METAL_MAX_SWAPCHAINS 10u

SystemDataPool<MetalSwapChainData, MetalSwapChainDataFull> metalSwapChainPool;

void InitMetalSwapChainMemory()
{
    if (!metalSwapChainPool.Storage)
    {
        metalSwapChainPool = SystemCreateDataPool<MetalSwapChainData, MetalSwapChainDataFull>(MetalGraphicsMemoryArena, METAL_MAX_SWAPCHAINS);
    }
}

MetalSwapChainData* GetMetalSwapChainData(ElemSwapChain swapChain)
{
    return SystemGetDataPoolItem(metalSwapChainPool, swapChain);
}

MetalSwapChainDataFull* GetMetalSwapChainDataFull(ElemSwapChain swapChain)
{
    return SystemGetDataPoolItemFull(metalSwapChainPool, swapChain);
}

ElemSwapChain MetalCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, const ElemSwapChainOptions* options)
{
    InitMetalSwapChainMemory();

    auto commandQueueDataFull = GetMetalCommandQueueDataFull(commandQueue);
    SystemAssert(commandQueueDataFull);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(commandQueueDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto windowData = GetMacOSWindowData(window);
    SystemAssert(windowData);
    
    // TODO: Code dependent on MacOS.
    auto metalLayer = NS::TransferPtr(CA::MetalLayer::layer());
    windowData->WindowHandle->contentView()->setLayer(metalLayer.get());

    auto windowRenderSize = ElemGetWindowRenderSize(window);
    auto width = windowRenderSize.Width;
    auto height = windowRenderSize.Height;
    auto format = MTL::PixelFormatBGRA8Unorm_sRGB; // TODO: Enumerate compatible formats first
    auto maximumFrameLatency = 1u;
 
    if (options)
    {
        if (options->Width != 0)
        {
            width = options->Width;
        }

        if (options->Height != 0)
        {
            height = options->Height;
        }

        if (options->Format == ElemSwapChainFormat_HighDynamicRange)
        {
            format = MTL::PixelFormatBGR10A2Unorm;
        }

        // TODO: Check boundaries
        if (options->MaximumFrameLatency != 0)
        {
            maximumFrameLatency = options->MaximumFrameLatency;
        }
    }

    metalLayer->setDevice(graphicsDeviceData->Device.get());
    metalLayer->setPixelFormat(format);
    metalLayer->setFramebufferOnly(true);
    metalLayer->setDrawableSize(CGSizeMake(width, height));
    
    auto waitSemaphore = dispatch_semaphore_create(maximumFrameLatency);

    auto handle = SystemAddDataPoolItem(metalSwapChainPool, {
        .DeviceObject = metalLayer,
        .WaitSemaphore = waitSemaphore,
        .CommandQueue = commandQueue,
        .GraphicsDevice = commandQueueDataFull->GraphicsDevice
    }); 

    SystemAddDataPoolItemFull(metalSwapChainPool, handle, {
    });

    return handle;
}

void MetalFreeSwapChain(ElemSwapChain swapChain)
{
    auto swapChainData = GetMetalSwapChainData(swapChain);
    SystemAssert(swapChainData);

    if (swapChainData->BackBufferTexture)
    {
        MetalFreeTexture(swapChainData->BackBufferTexture);
    }

    if (swapChainData->BackBufferDrawable)
    {
        swapChainData->BackBufferDrawable.reset();
    }

    if (swapChainData->DeviceObject)
    {
        swapChainData->DeviceObject.reset();
    }
}

void MetalResizeSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height)
{    
    auto swapChainData = GetMetalSwapChainData(swapChain);
    SystemAssert(swapChainData);

    swapChainData->DeviceObject->setDrawableSize(CGSizeMake(width, height));
}

ElemTexture MetalGetSwapChainBackBufferTexture(ElemSwapChain swapChain)
{
    auto swapChainData = GetMetalSwapChainData(swapChain);
    SystemAssert(swapChainData);

    if (!swapChainData->BackBufferDrawable.get())
    {
        SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "Backbuffer drawable is null.");
        return ELEM_HANDLE_NULL;
    }

    return swapChainData->BackBufferTexture;
}

void MetalPresentSwapChain(ElemSwapChain swapChain)
{
    auto swapChainData = GetMetalSwapChainData(swapChain);
    SystemAssert(swapChainData);

    auto commandQueueData = GetMetalCommandQueueData(swapChainData->CommandQueue);
    SystemAssert(commandQueueData);

    // HACK: Can we avoid creating an empty command buffer?
    auto commandBuffer = NS::RetainPtr(commandQueueData->DeviceObject->commandBufferWithUnretainedReferences());

    if (!commandBuffer.get())
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Error while creating command buffer object.");
        return;
    }
    
    if (!swapChainData->BackBufferDrawable.get())
    {
        SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "Backbuffer drawable is null.");
        return;
    }
    
    auto drawable = swapChainData->BackBufferDrawable;
   
    commandBuffer->addScheduledHandler([drawable](MTL::CommandBuffer* commandBuffer)
    {
        drawable->present();
    });
    
    commandBuffer->addCompletedHandler([swapChainData](MTL::CommandBuffer* commandBuffer)
    {
        dispatch_semaphore_signal(swapChainData->WaitSemaphore);
    }); 

    if (MetalDebugLayerEnabled)
    {
        commandBuffer->setLabel(MTLSTR("PresentSwapChainCommandBuffer"));
    }

    commandBuffer->commit();
}

void MetalWaitForSwapChainOnCpu(ElemSwapChain swapChain)
{
    auto swapChainData = GetMetalSwapChainData(swapChain);
    SystemAssert(swapChainData);

    dispatch_semaphore_wait(swapChainData->WaitSemaphore, DISPATCH_TIME_FOREVER);

    if (swapChainData->BackBufferDrawable)
    {
        swapChainData->BackBufferDrawable.reset();
    }

    if (swapChainData->BackBufferTexture)
    {
        MetalFreeTexture(swapChainData->BackBufferTexture);
    }

    auto nextMetalDrawable = NS::RetainPtr(swapChainData->DeviceObject->nextDrawable());
        
    if (!nextMetalDrawable.get())
    {
        SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "Cannot acquire a back buffer.");
        return;
    }

    swapChainData->BackBufferDrawable = nextMetalDrawable;
    swapChainData->BackBufferTexture = CreateMetalTextureFromResource(swapChainData->GraphicsDevice, NS::TransferPtr(swapChainData->BackBufferDrawable->texture()), true);
}
