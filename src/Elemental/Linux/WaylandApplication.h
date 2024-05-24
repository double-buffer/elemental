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
extern wl_output* WaylandOutput;
extern wl_compositor* WaylandCompositor;
extern libdecor* WaylandLibdecor;
extern uint32_t WaylandOutputRefreshRate;
extern float WaylandOutputScale;

extern uint64_t WaylandPerformanceCounterStart;
extern uint64_t WaylandPerformanceCounterFrequencyInSeconds;

void AddWaylandInitHandler(WaylandInitHandler handler);
void RemoveWaylandInitHandler(WaylandInitHandler handler) {}
void WaylandSetDefaultCursor(wl_pointer* pointer, uint32_t serial);

void WaylandOutputGeometryHandler(void* data, wl_output* output, int x, int y, int physical_width, int physical_height, int subpixel, const char* make, const char* model, int transform) {}
void WaylandOutputModeHandler(void* data, wl_output* output, uint32_t flags, int width, int height, int refresh);
void WaylandOutputDoneHandler(void* data, wl_output* output) {}
void WaylandOutputScaleHandler(void* data, wl_output* output, int32_t factor);

const wl_output_listener WaylandOutputListener =
{
    .geometry = WaylandOutputGeometryHandler,
    .mode = WaylandOutputModeHandler,
    .done = WaylandOutputDoneHandler,
    .scale = WaylandOutputScaleHandler
};

void WaylandLibdecorErrorHandler(libdecor* context, libdecor_error error, const char* message);

libdecor_interface WaylandLibdecorListener
{
    .error = WaylandLibdecorErrorHandler
};

void WaylandRegistryHandler(void* data, wl_registry* registry, uint32_t id, const char* interface, uint32_t version);

wl_registry_listener WaylandRegistryListener = 
{ 
    .global = WaylandRegistryHandler 
};
