#pragma once

#include "Elemental.h"

struct GtkWindowData
{
    GtkWidget* GtkWindow;
    GtkWidget* GtkContent;
    GdkDisplay* GdkDisplay;
    GdkSurface* GdkSurface;
    wl_display* WaylandDisplay;
    wl_surface* WaylandSurfaceParent;
    wl_surface* WaylandSurface;
    wl_subsurface* WaylandSubSurface;
    uint32_t MonitorRefreshRate;
    bool IsClosed;
};

struct GtkWindowDataFull
{
    uint32_t reserved;
};

GtkWindowData* GetGtkWindowData(ElemWindow window);
GtkWindowDataFull* GetGtkWindowDataFull(ElemWindow window);
