#include "MacOSWindow.h"
#include "MacOSApplication.h"
#include "SystemDataPool.h"

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

    SystemAddDataPoolItemFull(windowDataPool, handle, {
        .Width = (uint32_t)width,
        .Height = (uint32_t)height,
        .UIScale = 1.0f // TODO
    });
    
    return handle;
}

ElemAPI void ElemFreeWindow(ElemWindow window)
{
    SystemRemoveDataPoolItem(windowDataPool, window);
}

ElemAPI ElemWindowSize ElemGetWindowRenderSize(ElemWindow window)
{
    auto windowData = GetMacOSWindowData(window);
    auto contentView = windowData->WindowHandle->contentView();
    auto screen = windowData->WindowHandle->screen();
    
    auto mainScreenScaling = screen ? screen->backingScaleFactor() : 1.0f;
    auto contentViewSize = contentView->frame().size;
    
    return
    {
        .Width = (uint32_t)(contentViewSize.width * mainScreenScaling),
        .Height = (uint32_t)(contentViewSize.height * mainScreenScaling),
        .UIScale = (float)mainScreenScaling,
        .WindowState = ElemWindowState_Normal // TODO:
    };
}

ElemAPI void ElemSetWindowTitle(ElemWindow window, const char* title)
{
}

ElemAPI void ElemSetWindowState(ElemWindow window, ElemWindowState windowState)
{
}

/*

DllExport void* Native_CreateWindow(MacOSApplication* nativeApplication, NativeWindowOptions* options)
{
    CGRect windowSize =
    {
        .origin = { .x = 0.0, .y = 0.0 },
        .size = { .width = (double)options->Width, .height = (double)options->Height }
    };

    auto windowHandle = NS::TransferPtr(NS::Window::alloc()->init(
        windowSize,
        NS::WindowStyleMaskClosable | NS::WindowStyleMaskTitled | NS::WindowStyleMaskMiniaturizable | NS::WindowStyleMaskResizable,
        NS::BackingStoreBuffered,
        false ));

    //SystemLogDebugMessage(LogMessageCategory_NativeApplication, "Title %s", options->Title);

    // BUG: String conversion is not working for now :(
    auto titleTest = "Test Title TEMP";

    windowHandle->setTitle(NS::String::string(titleTest, NS::StringEncoding::UTF8StringEncoding));
    //windowHandle->setTitle(NS::String::string((const char*)options->Title, NS::StringEncoding::UnicodeStringEncoding));
    windowHandle->center();
    windowHandle->makeKeyAndOrderFront(nullptr);

    auto window = new MacOSWindow();
    window->WindowHandle = windowHandle;
    
    Native_SetWindowState(window, options->WindowState);

    return window;
}

DllExport void Native_FreeWindow(MacOSWindow* window)
{
    delete window;
}

DllExport NativeWindowSize Native_GetWindowRenderSize(MacOSWindow* nativeWindow)
{
    auto result = NativeWindowSize();

    auto contentView = nativeWindow->WindowHandle->contentView();
    auto screen = nativeWindow->WindowHandle->screen();
    
    auto mainScreenScaling = screen ? screen->backingScaleFactor() : 1.0f;
    auto contentViewSize = contentView->frame().size;
    
    result.Width = (uint32_t)(contentViewSize.width * mainScreenScaling);
    result.Height = (uint32_t)(contentViewSize.height * mainScreenScaling);
    result.UIScale = mainScreenScaling;
    result.WindowState = NativeWindowState_Normal;

    if (screen)
    {
        auto screenSize = screen->frame().size;

        if (nativeWindow->WindowHandle->miniaturized())
        {
            result.WindowState = NativeWindowState_Minimized;
        }
        else if (contentViewSize.width == screenSize.width && contentViewSize.height == screenSize.height)
        {
            result.WindowState = NativeWindowState_FullScreen;
        }
        else if (nativeWindow->WindowHandle->zoomed())
        {
            result.WindowState = NativeWindowState_Maximized;
        }
    }

    return result;
}

DllExport void Native_SetWindowTitle(MacOSWindow* nativeWindow, wchar_t* title)
{
    // BUG: String conversion is not working for now :(
    //nativeWindow->WindowHandle->setTitle(NS::String::string((const char*)title, NS::StringEncoding::UTF16StringEncoding));
}
    
DllExport void Native_SetWindowState(MacOSWindow* window, NativeWindowState windowState)
{
    auto contentView = window->WindowHandle->contentView();
    auto screen = window->WindowHandle->screen();

    if (!contentView || !screen)
    {
        return;
    }
    
    auto contentViewSize = contentView->frame().size;
    auto screenSize = screen->frame().size;

    if (window->WindowHandle->miniaturized() && windowState != NativeWindowState_Minimized)
    {
        window->WindowHandle->deminiaturize(nullptr);
    }
    else if (contentViewSize.width == screenSize.width && contentViewSize.height == screenSize.height && windowState != NativeWindowState_FullScreen)
    {
        window->WindowHandle->toggleFullScreen(nullptr);
    }
    else if (window->WindowHandle->zoomed() && windowState != NativeWindowState_Maximized)
    {
        window->WindowHandle->zoom(nullptr);
    }

    if (windowState == NativeWindowState_Maximized && !window->WindowHandle->zoomed())
    {
        window->WindowHandle->zoom(nullptr);
    }
    else if (windowState == NativeWindowState_FullScreen && contentViewSize.width != screenSize.width && contentViewSize.height != screenSize.height)
    {
        window->WindowHandle->toggleFullScreen(nullptr);
    }
    else if (windowState == NativeWindowState_Minimized && !window->WindowHandle->miniaturized())
    {
        window->WindowHandle->miniaturize(nullptr);
    }
}
*/
