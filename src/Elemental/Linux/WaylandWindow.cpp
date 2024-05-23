#include "WaylandWindow.h"
#include "WaylandApplication.h"
#include "WaylandInputs.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

// TODO: to remove?
struct WindowWaylandCallbackParameter
{
    ElemWindow Window;
};

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

static void
handle_xdg_toplevel_configure(void *data, struct xdg_toplevel *toplevel,
                              int32_t width, int32_t height,
                              struct wl_array *states)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Application, "XDG toplevel configure: %d, %d", width, height);
}

static void
handle_xdg_toplevel_close(void *data, struct xdg_toplevel *toplevel)
{
    auto windowData = (WaylandWindowData*)data;
    windowData->IsClosed = true;
    SystemLogDebugMessage(ElemLogMessageCategory_Application, "XDG toplevel close");
}

void handle_xdg_toplevel_configure_bounds(void *data,
				 struct xdg_toplevel *xdg_toplevel,
				 int32_t width,
				 int32_t height)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Application, "XDG toplevel configure bounds: %d, %d", width, height);
}

void handle_xdg_toplevel_wm_capabilities(void *data,
				struct xdg_toplevel *xdg_toplevel,
				struct wl_array *capabilities)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Application, "XDG toplevel wm capabilities");
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    handle_xdg_toplevel_configure,
    handle_xdg_toplevel_close,
    handle_xdg_toplevel_configure_bounds,
    handle_xdg_toplevel_wm_capabilities
};

void handle_xdg_surface_configure(void* data, struct xdg_surface* surface, uint32_t serial)
{
    xdg_surface_ack_configure(surface, serial);
    SystemLogDebugMessage(ElemLogMessageCategory_Application, "XDG surface configure");
}

static const struct xdg_surface_listener xdg_surface_listener = {
   handle_xdg_surface_configure,
};

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

    auto waylandSurface = wl_compositor_create_surface(WaylandCompositor);
    auto xdgSurface = xdg_wm_base_get_xdg_surface(WaylandShell, waylandSurface);

    xdg_surface_add_listener(xdgSurface, &xdg_surface_listener, waylandSurface);

    auto xdgToplevel = xdg_surface_get_toplevel(xdgSurface);
    xdg_toplevel_set_title(xdgToplevel, title);
    
    wl_surface_set_buffer_scale(waylandSurface, 2);
    
    //xdg_toplevel_set_fullscreen(xdgToplevel, nullptr);

     /*
    WaylandWidget* window = gtk_window_new();
    
    gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(GlobalWaylandApplication));
    gtk_window_set_title(GTK_WINDOW(window), title);
    //gtk_window_set_default_size(GTK_WINDOW(window), width, height);

    WaylandWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), box);
    // TODO: If we use that, it will be considered the min size :(
    gtk_widget_set_size_request(box, width, height);

    gtk_window_present(GTK_WINDOW(window));

    auto controller = gtk_shortcut_controller_new();
    gtk_widget_add_controller(window, controller);
    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller),
          gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_F11, GDK_NO_MODIFIER_MASK),
                           gtk_callback_action_new(ToggleWaylandFullScreen, NULL, NULL)));

    auto gdkSurface = gtk_native_get_surface(gtk_widget_get_native(window));
    auto gdkDisplay = gdk_surface_get_display(gdkSurface);
    auto gdkMonitor = gdk_display_get_monitor_at_surface(gdkDisplay, gdkSurface);
    auto refreshRate = SystemRoundUp((float)gdk_monitor_get_refresh_rate(gdkMonitor) / 1000.0f);

    auto waylandDisplay = gdk_wayland_display_get_wl_display(gdkDisplay);

    wl_registry_listener waylandRegistryListener = 
    {
        WaylandRegistryHandler,
        WaylandRegistryRemover
    };

    wl_registry* waylandRegistry = wl_display_get_registry(waylandDisplay);
    wl_registry_add_listener(waylandRegistry, &waylandRegistryListener, nullptr);

    wl_display_dispatch(waylandDisplay);
    wl_display_roundtrip(waylandDisplay);

    SystemAssert(waylandSubCompositor);
    auto waylandSurface = gdk_wayland_surface_get_wl_surface(gdkSurface);
    auto WaylandCompositor = gdk_wayland_display_get_wl_compositor(gdkDisplay);

    auto contentWaylandSurface = wl_compositor_create_surface(WaylandCompositor);
    wl_surface_set_buffer_scale(contentWaylandSurface, 2);

    auto contentWaylandSubsurface = wl_subcompositor_get_subsurface(waylandSubCompositor, contentWaylandSurface, waylandSurface);
    wl_subsurface_set_desync(contentWaylandSubsurface);
*/
    auto handle = SystemAddDataPoolItem(windowDataPool, {
        .WaylandDisplay = WaylandDisplay,
        .WaylandSurface = waylandSurface,
        .MonitorRefreshRate = 120
        //.MonitorRefreshRate = (uint32_t)refreshRate
    }); 

    SystemAddDataPoolItemFull(windowDataPool, handle, {
    });

    ElemSetWindowState(handle, windowState);

    auto windowData = GetWaylandWindowData(handle);
    xdg_toplevel_add_listener(xdgToplevel, &xdg_toplevel_listener, windowData);

    wl_surface_commit(waylandSurface);
    wl_display_roundtrip(WaylandDisplay);

    InitWaylandInputs(handle);
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

/*
    auto width = SystemMax((uint32_t)gtk_widget_get_width(windowData->WaylandContent), 1u);
    auto height = SystemMax((uint32_t)gtk_widget_get_height(windowData->WaylandContent), 1u);
    auto uiScale = (float)gdk_surface_get_scale_factor(windowData->GdkSurface);

    auto state = gdk_toplevel_get_state(GDK_TOPLEVEL(windowData->GdkSurface));

    auto windowState = ElemWindowState_Normal;

    if (state & GDK_TOPLEVEL_STATE_MINIMIZED)
    {
        windowState = ElemWindowState_Minimized;
    }
    else if (state & GDK_TOPLEVEL_STATE_FULLSCREEN)
    {
        windowState = ElemWindowState_FullScreen;
    }
    else if (state & GDK_TOPLEVEL_STATE_MAXIMIZED)
    {
        windowState = ElemWindowState_Maximized;
    }

    auto inputPoint = GRAPHENE_POINT_INIT_ZERO;
    graphene_point_t contentOffset;
    AssertIfFailed(gtk_widget_compute_point(windowData->WaylandContent, windowData->WaylandWindow, &inputPoint, &contentOffset));
    
    double offsetX, offsetY;
    gtk_native_get_surface_transform(gtk_widget_get_native(windowData->WaylandWindow), &offsetX, &offsetY);
    wl_subsurface_set_position(windowData->WaylandSubSurface, contentOffset.x + offsetX, contentOffset.y + offsetY);

    // TODO: Is it needed?
    //auto waylandRegion = wl_compositor_create_region(WaylandCompositor);
    //wl_region_add(waylandRegion, 0, 0, width, height);
    //wl_surface_opaque_region();
    //s.set_opaque_region(r)

    // TODO: Update monitor refresh rate?
*/
    return 
    {
        .Width = 1280,
        .Height = 720,
        .UIScale = 1.0f,
        //.WindowState = windowState
    };
}

ElemAPI void ElemSetWindowTitle(ElemWindow window, const char* title)
{
    auto windowData = GetWaylandWindowData(window);
    SystemAssert(windowData);
    
    //gtk_window_set_title(GTK_WINDOW(windowData->WaylandWindow), title);
}

ElemAPI void ElemSetWindowState(ElemWindow window, ElemWindowState windowState)
{
    auto windowData = GetWaylandWindowData(window);
    SystemAssert(windowData);

    SystemLogDebugMessage(ElemLogMessageCategory_Application, "SetWindowState");
/*
    auto state = gdk_toplevel_get_state(GDK_TOPLEVEL(windowData->GdkSurface));

    if (state & GDK_TOPLEVEL_STATE_MINIMIZED && windowState != ElemWindowState_Minimized)
    {
        gtk_window_unminimize(GTK_WINDOW(windowData->WaylandWindow));
    }
    else if (state & GDK_TOPLEVEL_STATE_FULLSCREEN && windowState != ElemWindowState_FullScreen)
    {
        gtk_window_unfullscreen(GTK_WINDOW(windowData->WaylandWindow));
    }
    else if (state & GDK_TOPLEVEL_STATE_MAXIMIZED && windowState != ElemWindowState_Maximized)
    {
        gtk_window_unmaximize(GTK_WINDOW(windowData->WaylandWindow));
    }

    if (windowState == ElemWindowState_Maximized && !(state & GDK_TOPLEVEL_STATE_MAXIMIZED))
    {
        gtk_window_maximize(GTK_WINDOW(windowData->WaylandWindow));
    }
    else if (windowState == ElemWindowState_FullScreen && !(state & GDK_TOPLEVEL_STATE_FULLSCREEN))
    {
        gtk_window_fullscreen(GTK_WINDOW(windowData->WaylandWindow));
    }
    else if (windowState == ElemWindowState_Minimized && !(state & GDK_TOPLEVEL_STATE_MINIMIZED))
    {
        gtk_window_minimize(GTK_WINDOW(windowData->WaylandWindow));
    }*/
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
