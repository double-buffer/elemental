#pragma once
#include "PreCompiledHeader.h"

struct Win32Window
{
    HWND WindowHandle;
    WINDOWPLACEMENT WindowPlacement;
    uint32_t Width;
    uint32_t Height;
    float_t UIScale;
};

DllExport void Native_SetWindowState(Win32Window *window, NativeWindowState windowState);