#include "MacOSWindow.h"
#include "MacOSApplication.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"

SystemDataPool<MacOSWindowData, MacOSWindowDataFull> windowDataPool;

void InitWindowMemory()
{
    if (!windowDataPool.Storage)
    {
        windowDataPool = SystemCreateDataPool<MacOSWindowData, MacOSWindowDataFull>(ApplicationMemoryArena, 10);
    }
}

MacOSWindowData* GetMacOSWindowData(ElemWindow window)
{
    return SystemGetDataPoolItem(windowDataPool, window);
}

MacOSWindowDataFull* GetMacOSWindowDataFull(ElemWindow window)
{
    return SystemGetDataPoolItemFull(windowDataPool, window);
}

ElemAPI ElemWindow ElemCreateWindow(ElemApplication application, const ElemWindowOptions* options)
{
    InitWindowMemory();

    auto title = "Elemental Window";
    auto width = 1280;
    auto height = 720;
    auto windowState = ElemWindowState_Normal;

    if (options != nullptr)
    {
        if (options->Title)
        {
            title = options->Title;
        }

        if (options->Width != 0)
        {
            width = options->Width;
        }

        if (options->Height != 0)
        {
            height = options->Height;
        }
        
        if (options->WindowState != 0)
        {
            windowState = options->WindowState;
        }
    }

    CGRect windowSize =
    {
        .origin = { .x = 0.0, .y = 0.0 },
        .size = { .width = (double)width, .height = (double)height }
    };

    auto windowHandle = NS::TransferPtr(NS::Window::alloc()->init(
        windowSize,
        NS::WindowStyleMaskClosable | NS::WindowStyleMaskTitled | NS::WindowStyleMaskMiniaturizable | NS::WindowStyleMaskResizable,
        NS::BackingStoreBuffered,
        false ));

    windowHandle->setTitle(NS::String::string(title, NS::StringEncoding::UTF8StringEncoding));
    //windowHandle->center();
    windowHandle->makeKeyAndOrderFront(nullptr);

    //auto applicationDataFull = GetMacOSApplicationDataFull(application);
    //SystemAssertReturnNullHandle(applicationDataFull);
    //applicationDataFull->WindowCount++;

    auto handle = SystemAddDataPoolItem(windowDataPool, {
        .WindowHandle = windowHandle
    }); 
    
    //auto windowDelegate = new MacOSWindowDelegate(handle);
    //windowHandle->setDelegate(windowDelegate);

    SystemAddDataPoolItemFull(windowDataPool, handle, {
        .Width = (uint32_t)width,
        .Height = (uint32_t)height,
        .UIScale = 1.0f,
        //.WindowDelegate = windowDelegate,
        .Application = application,
        .ClosingCalled = false
    });

    ElemSetWindowState(handle, windowState);
    
    return handle;
}

ElemAPI void ElemFreeWindow(ElemWindow window)
{
    auto windowData = GetMacOSWindowData(window);
    SystemAssert(windowData);

    auto windowDataFull = GetMacOSWindowDataFull(window);
    SystemAssert(windowDataFull);

    SystemRemoveDataPoolItem(windowDataPool, window);

    if (!windowDataFull->ClosingCalled)
    {
        windowData->WindowHandle->close();
    }

    auto applicationDataFull = GetMacOSApplicationDataFull(windowDataFull->Application);
    SystemAssert(applicationDataFull);
    applicationDataFull->WindowCount--;

    if (applicationDataFull->WindowCount == 0)
    {
        applicationDataFull->Status = ElemApplicationStatus_Closing;
    }

    windowData->WindowHandle.reset();
}

ElemAPI ElemWindowSize ElemGetWindowRenderSize(ElemWindow window)
{
    auto windowData = GetMacOSWindowData(window);
    SystemAssert(windowData);
/*
    auto contentView = windowData->WindowHandle->contentView();
    auto screen = windowData->WindowHandle->screen();

    auto mainScreenScaling = screen ? screen->backingScaleFactor() : 1.0f;
    auto contentViewSize = contentView->frame().size;

    auto width = (uint32_t)(contentViewSize.width * mainScreenScaling);
    auto height = (uint32_t)(contentViewSize.height * mainScreenScaling);
    auto windowState = ElemWindowState_Normal;

    if (windowData->WindowHandle->miniaturized())
    {
        width = 0;
        height = 0;
        windowState = ElemWindowState_Minimized;
    }
    else if (windowData->WindowHandle->styleMask() & NS::WindowStyleMaskFullScreen)
    {
        windowState = ElemWindowState_FullScreen;
    }
    else if (windowData->WindowHandle->zoomed())
    {
        windowState = ElemWindowState_Maximized;
    }*/

    auto width = 400u;
    auto height = 300u;
    auto mainScreenScaling = 1.0;
    auto windowState = ElemWindowState_Normal;

    return
    {
        .Width = width,
        .Height = height,
        .UIScale = (float)mainScreenScaling,
        .WindowState = windowState
    };
}

ElemAPI void ElemSetWindowTitle(ElemWindow window, const char* title)
{
    auto windowData = GetMacOSWindowData(window);
    SystemAssert(windowData);

    windowData->WindowHandle->setTitle(NS::String::string(title, NS::StringEncoding::UTF8StringEncoding));
}

ElemAPI void ElemSetWindowState(ElemWindow window, ElemWindowState windowState)
{
    auto windowData = GetMacOSWindowData(window);
    SystemAssert(windowData);
/*
    auto contentView = windowData->WindowHandle->contentView();
    SystemAssert(contentView);

    if (windowData->WindowHandle->miniaturized() && windowState != ElemWindowState_Minimized)
    {
        windowData->WindowHandle->deminiaturize(nullptr);
    }
    else if ((windowData->WindowHandle->styleMask() & NS::WindowStyleMaskFullScreen) && windowState != ElemWindowState_FullScreen)
    {
        windowData->WindowHandle->toggleFullScreen(nullptr);
    }
    else if (windowData->WindowHandle->zoomed() && windowState != ElemWindowState_Maximized)
    {
        windowData->WindowHandle->zoom(nullptr);
    }

    if (windowState == ElemWindowState_Maximized && !windowData->WindowHandle->zoomed())
    {
        windowData->WindowHandle->zoom(nullptr);
    }
    else if (windowState == ElemWindowState_FullScreen && !(windowData->WindowHandle->styleMask() & NS::WindowStyleMaskFullScreen))
    {
        windowData->WindowHandle->toggleFullScreen(nullptr);
    }
    else if (windowState == ElemWindowState_Minimized && !windowData->WindowHandle->miniaturized())
    {
        windowData->WindowHandle->miniaturize(nullptr);
    }*/
}
/*
MacOSWindowDelegate::MacOSWindowDelegate(ElemWindow window)
{
    _window = window;
}

MacOSWindowDelegate::~MacOSWindowDelegate()
{
}

void MacOSWindowDelegate::windowWillClose(NS::Notification* pNotification)
{            
    auto windowDataFull = GetMacOSWindowDataFull(_window);

    if (!windowDataFull)
    {
        return;
    }

    windowDataFull->ClosingCalled = true;

    ElemFreeWindow(_window);
}*/
