#pragma once

#include "Elemental.h"

struct WaylandWindowData
{
    wl_display* WaylandDisplay;
    wl_surface* WaylandSurface;
    uint32_t MonitorRefreshRate;
    bool IsClosed;
};

struct WaylandWindowDataFull
{
    uint32_t reserved;
};

WaylandWindowData* GetWaylandWindowData(ElemWindow window);
WaylandWindowDataFull* GetWaylandWindowDataFull(ElemWindow window);
