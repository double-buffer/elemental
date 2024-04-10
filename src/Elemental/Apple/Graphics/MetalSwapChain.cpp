#include "MetalSwapChain.h"
#include "MetalCommandList.h"
#include "MetalGraphicsDevice.h"
#include "MetalTexture.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"

#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
#include "../MacOSWindow.h"
#else
#include "../UIKitWindow.h"
#endif

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

    #if defined(TARGET_OS_OSX) && TARGET_OS_OSX
    auto windowData = GetMacOSWindowData(window);
    SystemAssert(windowData);

    auto metalLayer = NS::TransferPtr(CA::MetalLayer::layer());
    //windowData->WindowHandle->setContentViewtLayer(metalLayer.get());
    #else
    auto metalLayer = NS::TransferPtr(CA::MetalLayer::layer());
    auto windowData = GetUIKitWindowData(window);
    SystemAssert(windowData);
    #endif

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
/*
    auto handle = SystemAddDataPoolItem(metalSwapChainPool, {
        .DeviceObject = metalLayer,
        .WaitSemaphore = waitSemaphore,
        .CommandQueue = commandQueue,
        .GraphicsDevice = commandQueueDataFull->GraphicsDevice
    }); 

    SystemAddDataPoolItemFull(metalSwapChainPool, handle, {
    });

    return handle;*/
    return {};
}

ElemSwapChain MetalCreateSwapChain2(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options)
{
    InitMetalSwapChainMemory();

    auto commandQueueDataFull = GetMetalCommandQueueDataFull(commandQueue);
    SystemAssert(commandQueueDataFull);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(commandQueueDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);
    
    auto windowRenderSize = ElemGetWindowRenderSize(window);

    // TODO: Rename that to getapplewindowdata so we can have only one path?
    #if defined(TARGET_OS_OSX) && TARGET_OS_OSX
    auto metalView = NS::TransferPtr(MTK::View::alloc()->init(CGRectMake(0, 0, windowRenderSize.Width, windowRenderSize.Height), graphicsDeviceData->Device.get()));
    auto windowData = GetMacOSWindowData(window);
    SystemAssert(windowData);
    
    auto refreshRate = windowData->WindowHandle->screen()->maximumFramesPerSecond();
    windowData->WindowHandle->setContentView(metalView.get());
    #else
    auto metalView = NS::TransferPtr(MTK::View::alloc()->init(CGRectMake(0, 0, windowRenderSize.Width / windowRenderSize.UIScale, windowRenderSize.Height / windowRenderSize.UIScale), graphicsDeviceData->Device.get()));
    auto windowData = GetUIKitWindowData(window);
    SystemAssert(windowData);

    auto uiView = (UI::View*)metalView.get();
    uiView->setAutoresizingMask(UI::ViewAutoresizingFlexibleWidth | UI::ViewAutoresizingFlexibleHeight);
    windowData->ViewController->view()->addSubview(uiView);
    
    auto refreshRate = UI::Screen::mainScreen()->maximumFramesPerSecond();
    #endif
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Create swapchain with size: %dx%d@%f (%dHz)", windowRenderSize.Width, windowRenderSize.Height, windowRenderSize.UIScale, refreshRate);

    // TODO: Remove new
    auto metalViewDelegate = new MetalViewDelegate(updateHandler, options->UpdatePayload); // TODO: check options
    metalView->setDelegate(metalViewDelegate);

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

    metalView->setPreferredFramesPerSecond(refreshRate);
    metalView->setColorPixelFormat(format);
    metalView->setFramebufferOnly(true);
    metalView->setDrawableSize(CGSizeMake(width, height));

    auto waitSemaphore = dispatch_semaphore_create(maximumFrameLatency);

    auto handle = SystemAddDataPoolItem(metalSwapChainPool, {
        .DeviceObject = metalView,
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

ElemSwapChainInfo MetalGetSwapChainInfo(ElemSwapChain swapChain)
{
    return {};
}

void MetalResizeSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height)
{    
    if (width > 0 && height > 0)
    {
        auto swapChainData = GetMetalSwapChainData(swapChain);
        SystemAssert(swapChainData);

        swapChainData->DeviceObject->setDrawableSize(CGSizeMake(width, height));
    }
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

    //dispatch_semaphore_wait(swapChainData->WaitSemaphore, DISPATCH_TIME_FOREVER);

   /* if (swapChainData->BackBufferDrawable)
    {
        swapChainData->BackBufferDrawable.reset();
    }*/

    if (swapChainData->BackBufferTexture)
    {
        MetalFreeTexture(swapChainData->BackBufferTexture);
    }

    auto nextMetalDrawable = NS::RetainPtr(swapChainData->DeviceObject->currentDrawable()); // TODO: Review retain
        
    //if (!nextMetalDrawable.get())
    if (!nextMetalDrawable)
    {
        SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "Cannot acquire a back buffer.");
        return;
    }

    swapChainData->BackBufferDrawable = nextMetalDrawable;
    swapChainData->BackBufferTexture = CreateMetalTextureFromResource(swapChainData->GraphicsDevice, NS::RetainPtr(nextMetalDrawable->texture()), true);
}

MetalViewDelegate::MetalViewDelegate(ElemSwapChainUpdateHandlerPtr updateHandler, void* updatePayload)
{
    _updateHandler = updateHandler;
    _updatePayload = updatePayload;
}

void MetalViewDelegate::drawInMTKView(MTK::View* pView)
{
    if (_updateHandler)
    {
        _updateHandler(_updatePayload);
    }
}
