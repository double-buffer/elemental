#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

typedef void (*WaylandInitHandlerPtr)(ElemHandle handle);

struct WaylandInitHandler
{
    WaylandInitHandlerPtr Function;
    ElemHandle Handle;
    uint32_t NextIndex;
};

extern MemoryArena ApplicationMemoryArena;
extern wl_display* WaylandDisplay;
extern wl_compositor* WaylandCompositor;
extern xdg_wm_base* WaylandShell;

extern uint64_t WaylandPerformanceCounterStart;
extern uint64_t WaylandPerformanceCounterFrequencyInSeconds;

void AddWaylandInitHandler(WaylandInitHandler handler);
void RemoveWaylandInitHandler(WaylandInitHandler handler);

void WaylandXdgPingHandler(void* data, xdg_wm_base* shell, uint32_t serial);

xdg_wm_base_listener WaylandXdgListener = 
{
    .ping = WaylandXdgPingHandler
};
    

void WaylandRegistryHandler(void* data, wl_registry* registry, uint32_t id, const char* interface, uint32_t version);

wl_registry_listener WaylandRegistryListener = 
{ 
    .global = WaylandRegistryHandler 
};
