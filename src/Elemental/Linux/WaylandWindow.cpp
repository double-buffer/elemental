#include "WaylandWindow.h"
#include "WaylandApplication.h"
#include "SystemDataPool.h"
#include "SystemDictionary.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

SystemDataPool<WaylandWindowData, WaylandWindowDataFull> windowDataPool;

void InitWaylandWindowMemory()
{
    if (!windowDataPool.Storage)
    {
        windowDataPool = SystemCreateDataPool<WaylandWindowData, WaylandWindowDataFull>(ApplicationMemoryArena, 10);
    }
}

WaylandWindowData* GetWaylandWindowData(ElemWindow window)
{
    return SystemGetDataPoolItem(windowDataPool, window);
}

WaylandWindowDataFull* GetWaylandWindowDataFull(ElemWindow window)
{
    return SystemGetDataPoolItemFull(windowDataPool, window);
}

ElemAPI ElemWindow ElemCreateWindow(const ElemWindowOptions* options)
{
    InitWaylandWindowMemory();

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto width = 1280;
    auto height = 720;
    auto title = "Elemental Window";
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

    SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Welcome to wayland!");

    auto handle = SystemAddDataPoolItem(windowDataPool, {
    }); 

    SystemAddDataPoolItemFull(windowDataPool, handle, {
    });


    ElemSetWindowState(handle, windowState);

    return handle;
}

ElemAPI void ElemFreeWindow(ElemWindow window)
{
    auto windowData = GetWaylandWindowData(window);
    SystemAssert(windowData);

    auto windowDataFull = GetWaylandWindowDataFull(window);
    SystemAssert(windowDataFull);

    // TODO: Close application if last window

    SystemRemoveDataPoolItem(windowDataPool, window);
}

ElemAPI ElemWindowSize ElemGetWindowRenderSize(ElemWindow window)
{
    auto windowData = GetWaylandWindowData(window);
    SystemAssert(windowData);

    return 
    {
        //.Width = width,
        //.Height = height,
        //.UIScale = uiScale,
        //.WindowState = windowState
    };
}

ElemAPI void ElemSetWindowTitle(ElemWindow window, const char* title)
{
    auto windowData = GetWaylandWindowData(window);
    SystemAssert(windowData);
}

ElemAPI void ElemSetWindowState(ElemWindow window, ElemWindowState windowState)
{
    auto windowData = GetWaylandWindowData(window);
    SystemAssert(windowData);

    auto windowDataFull = GetWaylandWindowDataFull(window);
    SystemAssert(windowDataFull);
}
