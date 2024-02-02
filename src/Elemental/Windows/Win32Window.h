#pragma once
#include "ElementalOld.h"

struct Win32Window
{
    HWND WindowHandle;
    WINDOWPLACEMENT WindowPlacement;
    uint32_t Width;
    uint32_t Height;
    float UIScale;
};

DllExport void Native_SetWindowState(Win32Window *window, NativeWindowState windowState);
