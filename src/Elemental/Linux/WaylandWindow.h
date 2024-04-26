#pragma once

#include "Elemental.h"

struct WaylandWindowData
{
    uint32_t MonitorRefreshRate;
};

struct WaylandWindowDataFull
{
};

WaylandWindowData* GetWaylandWindowData(ElemWindow window);
WaylandWindowDataFull* GetWaylandWindowDataFull(ElemWindow window);
