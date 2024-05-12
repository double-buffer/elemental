#pragma once

#include "Elemental.h"

struct Win32WindowData
{
    HWND WindowHandle;
    uint32_t MonitorRefreshRate;
};

struct Win32WindowDataFull
{
    WINDOWPLACEMENT WindowPlacement;
    DWORD WindowStyle;
    DWORD WindowExStyle;
    HMONITOR Monitor;
};

Win32WindowData* GetWin32WindowData(ElemWindow window);
Win32WindowDataFull* GetWin32WindowDataFull(ElemWindow window);
