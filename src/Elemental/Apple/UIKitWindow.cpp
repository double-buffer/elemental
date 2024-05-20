#include "UIKitWindow.h"
#include "UIKitApplication.h"
#include "Inputs.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"

SystemDataPool<UIKitWindowData, SystemDataPoolDefaultFull> windowDataPool;

void InitWindowMemory()
{
    if (!windowDataPool.Storage)
    {
        windowDataPool = SystemCreateDataPool<UIKitWindowData>(ApplicationMemoryArena, 1);
    }
}

UIKitWindowData* GetUIKitWindowData(ElemWindow window)
{
    return SystemGetDataPoolItem(windowDataPool, window);
}

ElemAPI ElemWindow ElemCreateWindow(const ElemWindowOptions* options)
{
    InitWindowMemory();

    auto frame = UI::Screen::mainScreen()->bounds();
    auto windowHandle = NS::TransferPtr(UI::Window::alloc()->init(frame));
    auto viewController = NS::TransferPtr(UI::ViewController::alloc()->init(nullptr, nullptr));
    windowHandle->setRootViewController(viewController.get()); // TODO: Move that to window creation?
    windowHandle->makeKeyAndVisible();

    auto handle = SystemAddDataPoolItem(windowDataPool, {
        .WindowHandle = windowHandle,
        .ViewController = viewController
    }); 
    
    InitInputs(handle);
    
    return handle;
}

ElemAPI void ElemFreeWindow(ElemWindow window)
{
    auto windowData = GetUIKitWindowData(window);
    SystemAssert(windowData);

    SystemRemoveDataPoolItem(windowDataPool, window);
    windowData->WindowHandle.reset();
}

ElemAPI ElemWindowSize ElemGetWindowRenderSize(ElemWindow window)
{
    auto screen = UI::Screen::mainScreen();

    auto contentViewSizeNative = screen->nativeBounds().size;
    auto contentViewSize = screen->bounds().size;
    auto scaleNative = screen->nativeScale();

    auto width = (contentViewSize.width < contentViewSize.height) ? contentViewSizeNative.width : contentViewSizeNative.height;
    auto height = (contentViewSize.width < contentViewSize.height) ? contentViewSizeNative.height : contentViewSizeNative.width;

    return
    {
        .Width = (uint32_t)width,
        .Height = (uint32_t)height,
        .UIScale = (float)scaleNative,
        .WindowState = ElemWindowState_FullScreen
    };
}

ElemAPI void ElemSetWindowTitle(ElemWindow window, const char* title)
{
}

ElemAPI void ElemSetWindowState(ElemWindow window, ElemWindowState windowState)
{
}

ElemAPI void ElemShowWindowCursor(ElemWindow window)
{
}

ElemAPI void ElemHideWindowCursor(ElemWindow window)
{
}

ElemAPI ElemWindowCursorPosition ElemGetWindowCursorPosition(ElemWindow window)
{
    return {};
}
