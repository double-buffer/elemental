#include "MacOSWindow.h"
#include "MacOSApplication.h"
#include "Inputs.h"
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

ElemAPI ElemWindow ElemCreateWindow(const ElemWindowOptions* options)
{
    InitWindowMemory();

    auto title = "Elemental Window";
    auto width = 1280;
    auto height = 720;
    auto windowState = ElemWindowState_Normal;
    auto isCursorHidden = false;

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

        if (options->IsCursorHidden)
        {
            isCursorHidden = options->IsCursorHidden;
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
    windowHandle->center();
    windowHandle->makeKeyAndOrderFront(nullptr);

    auto handle = SystemAddDataPoolItem(windowDataPool, {
        .WindowHandle = windowHandle
    }); 

    // TODO: Avoid the new here
    auto windowDelegate = new MacOSWindowDelegate(handle);
    windowHandle->setDelegate(windowDelegate);

    SystemAddDataPoolItemFull(windowDataPool, handle, {
    });

    ElemSetWindowState(handle, windowState);

    if (isCursorHidden)
    {
        ElemHideWindowCursor(handle);
    }

    InitInputs(handle);
    
    return handle;
}

ElemAPI void ElemFreeWindow(ElemWindow window)
{
    auto windowData = GetMacOSWindowData(window);

    if (windowData == ELEM_HANDLE_NULL)
    {
        return;
    }

    SystemRemoveDataPoolItem(windowDataPool, window);
}

ElemAPI ElemWindowSize ElemGetWindowRenderSize(ElemWindow window)
{
    auto windowData = GetMacOSWindowData(window);
    SystemAssert(windowData);

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
    }
    
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
    }
}

MacOSWindowDelegate::MacOSWindowDelegate(ElemWindow window)
{
    _window = window;
}

MacOSWindowDelegate::~MacOSWindowDelegate()
{
}

void MacOSWindowDelegate::windowWillClose(NS::Notification* pNotification)
{            
    // BUG: This method is never executed when the user click on the close button
    // So sometimes the update swapchain method crash
    SystemLogDebugMessage(ElemLogMessageCategory_Application, "Window will close");
    auto windowData = GetMacOSWindowData(_window);

    if (!windowData)
    {
        return;
    }

    windowData->IsClosed = true;
}

ElemAPI void ElemShowWindowCursor(ElemWindow window)
{
    SystemAssert(window != ELEM_HANDLE_NULL);

    auto windowDataFull = GetMacOSWindowDataFull(window);
    SystemAssert(windowDataFull);

    if (windowDataFull->IsCursorHidden)
    {
        NS::Cursor::unhide();
        windowDataFull->IsCursorHidden = false;
    }
}

ElemAPI void ElemHideWindowCursor(ElemWindow window)
{
    SystemAssert(window != ELEM_HANDLE_NULL);

    auto windowDataFull = GetMacOSWindowDataFull(window);
    SystemAssert(windowDataFull);

    if (!windowDataFull->IsCursorHidden)
    {
        NS::Cursor::hide();
        windowDataFull->IsCursorHidden = true;
    }
}

ElemAPI ElemWindowCursorPosition ElemGetWindowCursorPosition(ElemWindow window)
{
    SystemAssert(window != ELEM_HANDLE_NULL);

    auto windowData = GetMacOSWindowData(window);
    SystemAssert(windowData);

    auto result = NS::Event::mouseLocation();
    result = windowData->WindowHandle->convertPointFromScreen(result);

    auto screen = windowData->WindowHandle->screen();
    auto mainScreenScaling = screen ? screen->backingScaleFactor() : 1.0f;
    auto clientRect = windowData->WindowHandle->contentView()->frame().size;
    result.x *= mainScreenScaling;
    result.y *= mainScreenScaling;

    auto clientMaxX = (long)(clientRect.width * mainScreenScaling) - 1;
    auto clientMaxY = (long)(clientRect.height * mainScreenScaling) - 1;

    result.x = SystemMax(0L, SystemMin((long)result.x, clientMaxX));
    result.y = clientMaxY - SystemMax(0L, SystemMin((long)result.y, clientMaxY));
  
    return { (uint32_t)result.x, (uint32_t)result.y };
}
