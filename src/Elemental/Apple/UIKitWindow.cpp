#include "UIKitWindow.h"
#include "UIKitApplication.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"

SystemDataPool<UIKitWindowData, UIKitWindowDataFull> windowDataPool;

void InitWindowMemory()
{
    if (!windowDataPool.Storage)
    {
        windowDataPool = SystemCreateDataPool<UIKitWindowData, UIKitWindowDataFull>(ApplicationMemoryArena, 10);
    }
}

UIKitWindowData* GetUIKitWindowData(ElemWindow window)
{
    return SystemGetDataPoolItem(windowDataPool, window);
}

UIKitWindowDataFull* GetUIKitWindowDataFull(ElemWindow window)
{
    return SystemGetDataPoolItemFull(windowDataPool, window);
}

ElemAPI ElemWindow ElemCreateWindow(ElemApplication application, const ElemWindowOptions* options)
{
    InitWindowMemory();

    auto frame = UI::Screen::mainScreen()->bounds();
    auto windowHandle = NS::TransferPtr(UI::Window::alloc()->init(frame));

    auto applicationDataFull = GetUIKitApplicationDataFull(application);
    SystemAssertReturnNullHandle(applicationDataFull);
    applicationDataFull->WindowCount++;

    auto handle = SystemAddDataPoolItem(windowDataPool, {
        .WindowHandle = windowHandle
    }); 
    
    SystemAddDataPoolItemFull(windowDataPool, handle, {
        .Width = (uint32_t)frame.size.width,
        .Height = (uint32_t)frame.size.height,
        .UIScale = 1.0f,
        .Application = application,
        .ClosingCalled = false
    });

    return handle;
}

ElemAPI void ElemFreeWindow(ElemWindow window)
{
    auto windowData = GetUIKitWindowData(window);
    SystemAssert(windowData);

    auto windowDataFull = GetUIKitWindowDataFull(window);
    SystemAssert(windowDataFull);

    SystemRemoveDataPoolItem(windowDataPool, window);

    auto applicationDataFull = GetUIKitApplicationDataFull(windowDataFull->Application);
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
    auto windowData = GetUIKitWindowData(window);
    SystemAssert(windowData);
    
    // TODO: Test with external screen
    // TODO: It seems we are not high res on iphone, we need a launch image?
    auto screen = UI::Screen::mainScreen();
    auto contentViewSize = screen->bounds().size;

    auto nativeBounds = screen->nativeBounds().size;
    auto nativeScale = screen->nativeScale();

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
