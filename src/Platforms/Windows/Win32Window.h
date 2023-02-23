#pragma once
#include "WindowsCommon.h"

struct Win32Window
{
    HWND WindowHandle;
    WINDOWPLACEMENT WindowPlacement;
    int Width;
    int Height;
    float UIScale;
};

DllExport void Native_SetWindowState(Win32Window *window, NativeWindowState windowState);