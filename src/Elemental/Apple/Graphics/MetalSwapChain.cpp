#include "MetalSwapChain.h"
#include "MetalCommandList.h"
#include "MetalGraphicsDevice.h"
#include "MetalResource.h"
#include "Inputs/Inputs.h"
#include "../Inputs.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

#define CAMetalLayer CA::MetalLayer
#define CAMetalDisplayLink CA::MetalDisplayLink
#include "ElementalSwiftLib.h"

struct Test
{
    // TODO: Change void*
    void* View;
    CA::MetalLayer* MetalLayer;
    CA::MetalDisplayLink* MetalDisplayLink;
};

#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
#include "../MacOSApplication.h"
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

void ResizeMetalSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height, float uiScale)
{    
    if (width == 0 || height == 0)
    {
        return;
    }

    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Resize Swapchain to %dx%d...", width, height);

    auto swapChainData = GetMetalSwapChainData(swapChain);
    SystemAssert(swapChainData);

    swapChainData->Width = width;
    swapChainData->Height = height;
    swapChainData->AspectRatio = (float)width / height;
    swapChainData->UIScale = uiScale;
    swapChainData->DeviceObject->setDrawableSize(CGSizeMake(width, height));
}

ElemSwapChain MetalCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options)
{
    InitMetalSwapChainMemory();

    auto commandQueueData = GetMetalCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(commandQueueData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);
    
    auto windowRenderSize = ElemGetWindowRenderSize(window);

    auto width = windowRenderSize.Width;
    auto height = windowRenderSize.Height;
    auto format = MTL::PixelFormatBGRA8Unorm_sRGB; // TODO: Enumerate compatible formats first
    auto frameLatency = 1u;
    void* updatePayload = nullptr;
 
    if (options)
    {
        if (options->Format == ElemSwapChainFormat_HighDynamicRange)
        {
            format = MTL::PixelFormatBGR10A2Unorm;
        }

        // TODO: Check boundaries
        if (options->FrameLatency != 0)
        {
            frameLatency = options->FrameLatency;
        }

        if (options->UpdatePayload)
        {
            updatePayload = options->UpdatePayload;
        }
    }

    // TODO: Remove new

    // BUG: On MacOS sometimes we have are capped at 60 fps only on builin display (after disable/enable pro motion, everything work)
    auto metalDisplayLinkHandler = new MetalDisplayLinkHandler(commandQueueData->GraphicsDevice, updateHandler, updatePayload);

    auto swiftResult = ElementalSwiftLib::createMetalView(frameLatency, window, (void*)TouchHandler); 
    auto metalViewResult = (Test*)&swiftResult; 
    auto metalLayer = NS::TransferPtr(metalViewResult->MetalLayer);

    auto metalDisplayLink = NS::TransferPtr(metalViewResult->MetalDisplayLink);
    metalDisplayLink->setDelegate(metalDisplayLinkHandler);

    // TODO: Rename that to getapplewindowdata so we can have only one path?
    #if defined(TARGET_OS_OSX) && TARGET_OS_OSX
    auto windowData = GetMacOSWindowData(window);
    SystemAssert(windowData);
    
    auto refreshRate = windowData->WindowHandle->screen()->maximumFramesPerSecond();
    windowData->WindowHandle->setContentView((NS::View*)metalViewResult->View);
    #else
    auto windowData = GetUIKitWindowData(window);
    SystemAssert(windowData);

    auto uiView = (UI::View*)metalViewResult->View;
    uiView->setAutoresizingMask(UI::ViewAutoresizingFlexibleWidth | UI::ViewAutoresizingFlexibleHeight);
    windowData->ViewController->view()->addSubview(uiView);
    
    auto refreshRate = UI::Screen::mainScreen()->maximumFramesPerSecond();
    #endif

    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Create swapchain with size: %dx%d@%f. (%dHz)", windowRenderSize.Width, windowRenderSize.Height, windowRenderSize.UIScale, refreshRate);

    metalLayer->setDevice(graphicsDeviceData->Device.get());
    metalLayer->setPixelFormat(format);
    metalLayer->setDrawableSize(CGSizeMake(width, height));
    metalLayer->setFramebufferOnly(true);

    auto handle = SystemAddDataPoolItem(metalSwapChainPool, {
        .DeviceObject = metalLayer,
        .Window = window,
        .CommandQueue = commandQueue,
        .GraphicsDevice = commandQueueData->GraphicsDevice,
        .CreationTimestamp = CA::CurrentMediaTime(),
        .PreviousTargetPresentationTimestamp = CA::CurrentMediaTime(),
        .Width = width,
        .Height = height,
        .AspectRatio = (float)width / height,
        .UIScale = windowRenderSize.UIScale,
        .Format = ElemGraphicsFormat_B8G8R8A8_SRGB // TODO: Temporary
    }); 

    SystemAddDataPoolItemFull(metalSwapChainPool, handle, {
        .MetalDisplayLink = metalDisplayLink,
        .MetalDisplayLinkHandler = metalDisplayLinkHandler
    });
    
    return handle;
}

void MetalFreeSwapChain(ElemSwapChain swapChain)
{
    auto swapChainData = GetMetalSwapChainData(swapChain);
    SystemAssert(swapChainData);

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
    auto swapChainData = GetMetalSwapChainData(swapChain);
    SystemAssert(swapChainData);

    return 
    {
        .Window = swapChainData->Window,
        .Width = swapChainData->Width,
        .Height = swapChainData->Height,
        .AspectRatio = swapChainData->AspectRatio,
        .UIScale = swapChainData->UIScale,
        .Format = swapChainData->Format
    };
}

void MetalSetSwapChainTiming(ElemSwapChain swapChain, uint32_t frameLatency, uint32_t targetFPS)
{
    // TODO: Not implemented yet
    SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "This feature is not yet implemented.");
}

void MetalPresentSwapChain(ElemSwapChain swapChain)
{
    auto swapChainData = GetMetalSwapChainData(swapChain);
    SystemAssert(swapChainData);
    
    swapChainData->PresentCalled = true;

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

    if (MetalDebugLayerEnabled)
    {
        commandBuffer->setLabel(MTLSTR("PresentSwapChainCommandBuffer"));
    }

    // TODO: Use present at time ? to control frame pacing?
    commandBuffer->presentDrawable(swapChainData->BackBufferDrawable.get());
    commandBuffer->commit();
}

MetalDisplayLinkHandler::MetalDisplayLinkHandler(ElemSwapChain swapChain, ElemSwapChainUpdateHandlerPtr updateHandler, void* updatePayload)
{
    _swapChain = swapChain;
    _updateHandler = updateHandler;
    _updatePayload = updatePayload;
}  

void MetalDisplayLinkHandler::metalDisplayLinkNeedsUpdate(CA::MetalDisplayLink* displayLink, CA::MetalDisplayLinkUpdate* update)
{
    auto swapChainData = GetMetalSwapChainData(_swapChain);
    SystemAssert(swapChainData);

    #if defined(TARGET_OS_OSX) && TARGET_OS_OSX
    auto windowData = GetMacOSWindowData(swapChainData->Window);

    if (ApplicationExited || windowData->IsClosed)
    {
        return;
    }
    #endif
    if (_updateHandler)
    {
        // TODO: If delta time is above a thresold, take the delta time based on target FPS
        auto deltaTime = update->targetPresentationTimestamp() - swapChainData->PreviousTargetPresentationTimestamp;
        swapChainData->PreviousTargetPresentationTimestamp = update->targetPresentationTimestamp();

        auto nextPresentTimestampInSeconds = update->targetPresentationTimestamp() - swapChainData->CreationTimestamp;

        auto sizeChanged = false;

        auto windowSize = ElemGetWindowRenderSize(swapChainData->Window);

        if (windowSize.Width > 0 && windowSize.Height > 0 && (windowSize.Width != swapChainData->Width || windowSize.Height != swapChainData->Height))
        {
            ResizeMetalSwapChain(_swapChain, windowSize.Width, windowSize.Height, windowSize.UIScale);
            sizeChanged = true;
        }

        swapChainData->PresentCalled = false;
        swapChainData->BackBufferDrawable = NS::RetainPtr(update->drawable());
        
        if (!swapChainData->BackBufferDrawable)
        {
            SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "Cannot acquire a back buffer.");
            return;
        }

        // BUG: On iOS, the drawable will have his size change on next update
        if (swapChainData->BackBufferDrawable->texture()->width() != swapChainData->Width || swapChainData->BackBufferDrawable->texture()->height() != swapChainData->Height)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Error size swapchain");
            return;
        }

        // TODO: Can we do something better than juste creating/destroying each time? 
        auto backBufferTexture = CreateMetalGraphicsResourceFromResource(swapChainData->GraphicsDevice, ElemGraphicsResourceType_Texture2D, ELEM_HANDLE_NULL, ElemGraphicsResourceUsage_RenderTarget, NS::RetainPtr(swapChainData->BackBufferDrawable->texture()), true);

        ElemSwapChainUpdateParameters updateParameters = 
        {
            .SwapChainInfo = MetalGetSwapChainInfo(_swapChain),
            .BackBufferRenderTarget = backBufferTexture,
            .DeltaTimeInSeconds = deltaTime,
            .NextPresentTimestampInSeconds = nextPresentTimestampInSeconds,
            .SizeChanged = sizeChanged
        };

        _updateHandler(&updateParameters, _updatePayload);
        ResetInputsFrame();

        if (!swapChainData->PresentCalled)
        {
            SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "Present was not called during update.");
            MetalPresentSwapChain(_swapChain);
        }
        
        MetalFreeGraphicsResource(backBufferTexture, nullptr);

        MetalProcessGraphicsResourceDeleteQueue(swapChainData->GraphicsDevice);
    }
}
