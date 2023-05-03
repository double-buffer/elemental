#pragma once

struct MacOSWindow
{
    NS::SharedPtr<NS::Window> WindowHandle;
    uint32_t Width;
    uint32_t Height;
    float_t UIScale;
};

DllExport void Native_SetWindowState(MacOSWindow* window, NativeWindowState windowState);