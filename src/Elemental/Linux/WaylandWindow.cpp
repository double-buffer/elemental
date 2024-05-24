#include "WaylandWindow.h"
#include "WaylandApplication.h"
#include "WaylandInputs.h"
#include "HidrawInputs.h"
#include "SystemDataPool.h"
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

// TODO: For the moment resizing the window with the cursor is very slow
void WaylandLibdecorFrameConfigureHandler(libdecor_frame* frame, libdecor_configuration* configuration, void* user_data)
{
    auto window = (ElemWindow)user_data;
    auto windowData = GetWaylandWindowData(window);
   
    libdecor_window_state windowState;

	if (libdecor_configuration_get_window_state(configuration, &windowState))
    {
        if (windowState & LIBDECOR_WINDOW_STATE_FULLSCREEN)
        {
            windowData->WindowState = ElemWindowState_FullScreen;
        }
        else if (windowState & LIBDECOR_WINDOW_STATE_MAXIMIZED)
        {
            windowData->WindowState = ElemWindowState_Maximized;
        }
        else if (windowState & LIBDECOR_WINDOW_STATE_SUSPENDED)
        {
            windowData->WindowState = ElemWindowState_Minimized;
        }
        else 
        {
            windowData->WindowState = ElemWindowState_Normal;
        }
    }

    int32_t width;
    int32_t height;

	if (libdecor_configuration_get_content_size(configuration, frame, &width, &height)) 
    {
		windowData->Width = width;
		windowData->Height = height;
	}
    
    auto state = libdecor_state_new(windowData->Width, windowData->Height);
	libdecor_frame_commit(frame, state, configuration);
	libdecor_state_free(state);
}

void WaylandLibdecorFrameCloseHandler(libdecor_frame* frame, void* user_data)
{
    auto window = (ElemWindow)user_data;
    auto windowData = GetWaylandWindowData(window);
    windowData->IsClosed = true;
}

ElemAPI ElemWindow ElemCreateWindow(const ElemWindowOptions* options)
{
    InitWaylandWindowMemory();

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto width = 1280u;
    auto height = 720u;
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

    auto handle = SystemAddDataPoolItem(windowDataPool, {
        .WaylandDisplay = WaylandDisplay,
        .Width = width,
        .Height = height,
        .Scale = WaylandOutputScale,
        .WindowState = ElemWindowState_Normal,
        .MonitorRefreshRate = WaylandOutputRefreshRate
    }); 

    SystemAddDataPoolItemFull(windowDataPool, handle, {
    });

    auto windowData = GetWaylandWindowData(handle);

    windowData->WaylandSurface = wl_compositor_create_surface(WaylandCompositor);
    wl_surface_set_buffer_scale(windowData->WaylandSurface, WaylandOutputScale);

    windowData->WaylandFrame = libdecor_decorate(WaylandLibdecor, windowData->WaylandSurface, &WaylandLibdecorFrameListener, (void*)handle);
    libdecor_frame_set_app_id(windowData->WaylandFrame, title);
	libdecor_frame_set_title(windowData->WaylandFrame, title);
	libdecor_frame_map(windowData->WaylandFrame);
   
    ElemSetWindowState(handle, windowState);
    InitWaylandInputs(handle);
    InitHidrawInputs(handle);
    return handle;
}

ElemAPI void ElemFreeWindow(ElemWindow window)
{
    auto windowData = GetWaylandWindowData(window);
    SystemAssert(windowData);

    SystemRemoveDataPoolItem(windowDataPool, window);
}

ElemAPI ElemWindowSize ElemGetWindowRenderSize(ElemWindow window)
{
    auto windowData = GetWaylandWindowData(window);
    SystemAssert(windowData);
    
    if (windowData->Scale != WaylandOutputScale)
    {
        wl_surface_set_buffer_scale(windowData->WaylandSurface, WaylandOutputScale);
        windowData->Scale = WaylandOutputScale;
    }

    return 
    {
        .Width = (uint32_t)(windowData->Width * windowData->Scale),
        .Height = (uint32_t)(windowData->Height * windowData->Scale),
        .UIScale = windowData->Scale,
        .WindowState = windowData->WindowState
    };
}

ElemAPI void ElemSetWindowTitle(ElemWindow window, const char* title)
{
    auto windowData = GetWaylandWindowData(window);
    SystemAssert(windowData);
    
	libdecor_frame_set_title(windowData->WaylandFrame, title);
}

ElemAPI void ElemSetWindowState(ElemWindow window, ElemWindowState windowState)
{
    auto windowData = GetWaylandWindowData(window);
    SystemAssert(windowData);

    if (windowData->WindowState == ElemWindowState_FullScreen)
    {
        libdecor_frame_unset_fullscreen(windowData->WaylandFrame);
    }
    else if (windowState == ElemWindowState_FullScreen && windowData->WindowState != ElemWindowState_FullScreen)
    {
        libdecor_frame_set_fullscreen(windowData->WaylandFrame, WaylandOutput);
    }

    if (windowState == ElemWindowState_Maximized && windowData->WindowState != ElemWindowState_Maximized)
    {
        libdecor_frame_set_maximized(windowData->WaylandFrame);
    }
    
    if (windowState == ElemWindowState_Minimized && windowData->WindowState != ElemWindowState_Minimized)
    {
        libdecor_frame_set_minimized(windowData->WaylandFrame);
    }
}

ElemAPI void ElemShowWindowCursor(ElemWindow window)
{
    SystemAssert(window != ELEM_HANDLE_NULL);

    auto windowDataFull = GetWaylandWindowDataFull(window);
    SystemAssert(windowDataFull);

    if (windowDataFull->IsCursorHidden)
    {
        windowDataFull->IsCursorHidden = false;
    }
}

ElemAPI void ElemHideWindowCursor(ElemWindow window)
{
    SystemAssert(window != ELEM_HANDLE_NULL);

    auto windowDataFull = GetWaylandWindowDataFull(window);
    SystemAssert(windowDataFull);

    if (!windowDataFull->IsCursorHidden)
    {
        windowDataFull->IsCursorHidden = true;
    }
}

ElemAPI ElemWindowCursorPosition ElemGetWindowCursorPosition(ElemWindow window)
{
    auto windowData = GetWaylandWindowData(window);
    SystemAssert(windowData);

    int32_t frameX;
    int32_t frameY;
    libdecor_frame_translate_coordinate(windowData->WaylandFrame, 0, 0, &frameX, &frameY); 

    uint32_t positionX = SystemMax(0, (int32_t)windowData->SurfaceCursorPositionX - frameX);
    uint32_t positionY = SystemMax(0, (int32_t)windowData->SurfaceCursorPositionY - frameY);

    return { positionX, positionY };
}
