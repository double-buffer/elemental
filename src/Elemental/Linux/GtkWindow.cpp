#include "GtkWindow.h"
#include "GtkApplication.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

SystemDataPool<GtkWindowData, GtkWindowDataFull> windowDataPool;

wl_subcompositor* waylandSubCompositor;

void InitGtkWindowMemory()
{
    if (!windowDataPool.Storage)
    {
        windowDataPool = SystemCreateDataPool<GtkWindowData, GtkWindowDataFull>(ApplicationMemoryArena, 10);
    }
}

GtkWindowData* GetGtkWindowData(ElemWindow window)
{
    return SystemGetDataPoolItem(windowDataPool, window);
}

GtkWindowDataFull* GetGtkWindowDataFull(ElemWindow window)
{
    return SystemGetDataPoolItemFull(windowDataPool, window);
}

void WaylandRegistryHandler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{ 
    //SystemLogDebugMessage(ElemLogMessageCategory_Application, "Got a registry event for %s id %d", interface, id);

    // TODO: Replace by system functions
    if (strcmp(interface, "wl_subcompositor") == 0) 
    {
        waylandSubCompositor = (wl_subcompositor*)wl_registry_bind(registry, id, &wl_subcompositor_interface, 1);
    }
}

void WaylandRegistryRemover(void *data, struct wl_registry *registry, uint32_t id)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Application, "Wayland registrery remove");
}

ElemAPI ElemWindow ElemCreateWindow(const ElemWindowOptions* options)
{
    InitGtkWindowMemory();

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

    GtkWidget* window = gtk_window_new();
    
    gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(GlobalGtkApplication));
    gtk_window_set_title(GTK_WINDOW(window), title);
    //gtk_window_set_default_size(GTK_WINDOW(window), width, height);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), box);
    // TODO: If we use that, it will be considered the min size :(
    gtk_widget_set_size_request(box, width, height);

    gtk_window_present(GTK_WINDOW(window));

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

    auto handle = SystemAddDataPoolItem(windowDataPool, {
        .GtkWindow = window,
        .GtkContent = box,
        .GdkSurface = gdkSurface,
        .WaylandDisplay = waylandDisplay,
        .WaylandSurfaceParent = waylandSurface,
        .WaylandSurface = contentWaylandSurface,
        .WaylandSubSurface = contentWaylandSubsurface,
        .MonitorRefreshRate = (uint32_t)refreshRate
    }); 

    SystemAddDataPoolItemFull(windowDataPool, handle, {
    });


    ElemSetWindowState(handle, windowState);

    return handle;
}

ElemAPI void ElemFreeWindow(ElemWindow window)
{
    auto windowData = GetGtkWindowData(window);
    SystemAssert(windowData);

    gtk_window_close(GTK_WINDOW(windowData->GtkWindow));

    SystemRemoveDataPoolItem(windowDataPool, window);
}

ElemAPI ElemWindowSize ElemGetWindowRenderSize(ElemWindow window)
{
    auto windowData = GetGtkWindowData(window);
    SystemAssert(windowData);

    auto width = SystemMax((uint32_t)gtk_widget_get_width(windowData->GtkContent), 1u);
    auto height = SystemMax((uint32_t)gtk_widget_get_height(windowData->GtkContent), 1u);
    auto uiScale = (float)gdk_surface_get_scale_factor(windowData->GdkSurface);
    auto windowState = ElemWindowState_Normal;

    auto inputPoint = GRAPHENE_POINT_INIT_ZERO;
    graphene_point_t contentOffset;
    AssertIfFailed(gtk_widget_compute_point(windowData->GtkContent, windowData->GtkWindow, &inputPoint, &contentOffset));
    
    double offsetX, offsetY;
    gtk_native_get_surface_transform(gtk_widget_get_native(windowData->GtkWindow), &offsetX, &offsetY);
    wl_subsurface_set_position(windowData->WaylandSubSurface, contentOffset.x + offsetX, contentOffset.y + offsetY);


    // BUG: For the moment we have a suboptimal swapchain in vulkan if we go fullscreen with offset 0
    // Use GDK fullscreen?
    //gtk_window_fullscreen(GTK_WINDOW(windowData->GtkWindow));
    // TODO: Is it needed?
    //auto waylandRegion = wl_compositor_create_region(WaylandCompositor);
    //wl_region_add(waylandRegion, 0, 0, width, height);
    //wl_surface_opaque_region();
    //s.set_opaque_region(r)

    // TODO: Update monitor refresh rate?

    return 
    {
        .Width = (uint32_t)(width * uiScale),
        .Height = (uint32_t)(height * uiScale),
        .UIScale = uiScale,
        .WindowState = windowState
    };
}

ElemAPI void ElemSetWindowTitle(ElemWindow window, const char* title)
{
    auto windowData = GetGtkWindowData(window);
    SystemAssert(windowData);
    
    gtk_window_set_title(GTK_WINDOW(windowData->GtkWindow), title);
}

ElemAPI void ElemSetWindowState(ElemWindow window, ElemWindowState windowState)
{
    /*auto windowData = GetGtkWindowData(window);
    SystemAssert(windowData);

    auto windowDataFull = GetGtkWindowDataFull(window);
    SystemAssert(windowDataFull);*/
}
