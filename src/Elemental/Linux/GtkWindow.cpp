#include "GtkWindow.h"
#include "GtkApplication.h"
#include "SystemDataPool.h"
#include "SystemDictionary.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

SystemDataPool<GtkWindowData, GtkWindowDataFull> windowDataPool;

// TODO: Temporary
wl_subcompositor* WaylandSubCompositor;

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

static void on_resize(GtkWidget *widget, int width, int height, gpointer user_data) {
    SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Resize");
    // Handle the resize event, you can adjust conditions or respond to the new size
    // For example, you might adjust internal drawing parameters or react to the size change
    g_print("Widget resized to: %d x %d\n", width, height);
}

void WaylandRegistryHandler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{ 
    SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Got a registry event for %s id %d", interface, id);

    // TODO: Replace by system functions
    if (strcmp(interface, "wl_subcompositor") == 0) 
    {
        WaylandSubCompositor = (wl_subcompositor*)wl_registry_bind(registry, id, &wl_subcompositor_interface, 1);
    }
}

void WaylandRegistryRemover(void *data, struct wl_registry *registry, uint32_t id)
{
    SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Wayland registrery remove");
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
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), box);

    gtk_window_present(GTK_WINDOW(window));

    auto gdkSurface = gtk_native_get_surface(gtk_widget_get_native(window));
    auto gdkDisplay = gdk_surface_get_display(gdkSurface);

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

    SystemAssert(WaylandSubCompositor);
    auto waylandSurface = gdk_wayland_surface_get_wl_surface(gdkSurface);
    auto WaylandCompositor = gdk_wayland_display_get_wl_compositor(gdkDisplay);

    auto contentWaylandSurface = wl_compositor_create_surface(WaylandCompositor);
    wl_surface_set_buffer_scale(contentWaylandSurface, 2);

    auto contentWaylandSubsurface = wl_subcompositor_get_subsurface(WaylandSubCompositor, contentWaylandSurface, waylandSurface);
    wl_subsurface_set_desync(contentWaylandSubsurface);

    auto handle = SystemAddDataPoolItem(windowDataPool, {
        .GtkWindow = window,
        .GtkContent = box,
        .GdkSurface = gdkSurface,
        .WaylandDisplay = waylandDisplay,
        .WaylandSurfaceParent = waylandSurface,
        .WaylandSurface = contentWaylandSurface,
        .WaylandSubSurface = contentWaylandSubsurface
    }); 

    SystemAddDataPoolItemFull(windowDataPool, handle, {
    });


    ElemSetWindowState(handle, windowState);

    return handle;
}

ElemAPI void ElemFreeWindow(ElemWindow window)
{
    /*auto windowData = GetGtkWindowData(window);
    SystemAssert(windowData);

    auto windowDataFull = GetGtkWindowDataFull(window);
    SystemAssert(windowDataFull);

    // TODO: Close application if last window

    SystemRemoveDataPoolItem(windowDataPool, window);*/
}

ElemAPI ElemWindowSize ElemGetWindowRenderSize(ElemWindow window)
{
    auto windowData = GetGtkWindowData(window);
    SystemAssert(windowData);

    auto width = SystemMax((uint32_t)gtk_widget_get_width(windowData->GtkContent), 10u);
    auto height = SystemMax((uint32_t)gtk_widget_get_height(windowData->GtkContent), 10u);
    auto uiScale = (float)gdk_surface_get_scale_factor(windowData->GdkSurface);
    auto windowState = ElemWindowState_Normal;

    double x, y;
    //gtk_widget_compute_point(
    gtk_widget_translate_coordinates(windowData->GtkContent, windowData->GtkWindow, 0, 0, &x, &y);

    // TODO: Get offset for window
    wl_subsurface_set_position(windowData->WaylandSubSurface, x + 14, y + 12);

    // TODO: Is it needed?
    //auto waylandRegion = wl_compositor_create_region(WaylandCompositor);
    //wl_region_add(waylandRegion, 0, 0, width, height);
    //wl_surface_opaque_region();
    //s.set_opaque_region(r)

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
