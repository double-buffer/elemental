#include "UIKitWindow.h"
#include "UIKitApplication.h"
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

ElemAPI ElemWindow ElemCreateWindow(ElemApplication application, const ElemWindowOptions* options)
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
    auto contentViewSize = screen->bounds().size;

    auto width = (uint32_t)(contentViewSize.width * screen->scale());
    auto height = (uint32_t)(contentViewSize.height * screen->scale());

    return
    {
        .Width = width,
        .Height = height,
        .UIScale = (float)screen->scale(),
        .WindowState = ElemWindowState_FullScreen
    };
}

ElemAPI void ElemSetWindowTitle(ElemWindow window, const char* title)
{
}

ElemAPI void ElemSetWindowState(ElemWindow window, ElemWindowState windowState)
{
}
