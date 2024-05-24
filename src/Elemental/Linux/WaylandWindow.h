#pragma once

#include "Elemental.h"

struct WaylandWindowData
{
    wl_display* WaylandDisplay;
    wl_surface* WaylandSurface;
    libdecor_frame* WaylandFrame;
    uint32_t Width;
    uint32_t Height;
    float Scale;
    ElemWindowState WindowState;
    uint32_t MonitorRefreshRate;
    bool IsClosed;
    uint32_t SurfaceCursorPositionX;
    uint32_t SurfaceCursorPositionY;
};

struct WaylandWindowDataFull
{
    bool IsCursorHidden;
    uint32_t reserved;
};

WaylandWindowData* GetWaylandWindowData(ElemWindow window);
WaylandWindowDataFull* GetWaylandWindowDataFull(ElemWindow window);

void WaylandLibdecorFrameConfigureHandler(libdecor_frame* frame, libdecor_configuration* configuration, void *user_data);
void WaylandLibdecorFrameCloseHandler(libdecor_frame* frame, void* user_data);
void WaylandLibdecorFrameCommitHandler(libdecor_frame* frame, void* user_data) {}
void WaylandLibdecorFrameDismissPopupHandler(libdecor_frame* frame, const char* seat_name, void* user_data) {}

libdecor_frame_interface WaylandLibdecorFrameListener = 
{
	.configure = WaylandLibdecorFrameConfigureHandler,
	.close = WaylandLibdecorFrameCloseHandler,
	.commit = WaylandLibdecorFrameCommitHandler,
	.dismiss_popup = WaylandLibdecorFrameDismissPopupHandler,
};
