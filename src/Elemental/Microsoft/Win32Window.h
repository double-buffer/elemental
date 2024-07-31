#pragma once

#include "Elemental.h"

struct Win32WindowData
{
    HWND WindowHandle;
    uint32_t MonitorRefreshRate;
    ComPtr<IDXGIOutput> Output;
};

struct Win32WindowDataFull
{
    WINDOWPLACEMENT WindowPlacement;
    DWORD WindowStyle;
    DWORD WindowExStyle;
    HMONITOR Monitor;
    bool IsCursorHidden;
};

Win32WindowData* GetWin32WindowData(ElemWindow window);
Win32WindowDataFull* GetWin32WindowDataFull(ElemWindow window);
