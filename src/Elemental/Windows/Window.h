#pragma once

struct WindowsWindow
{
    HWND WindowHandle;
};

struct WindowsWindowFull
{
    WINDOWPLACEMENT WindowPlacement;
    uint32_t Width;
    uint32_t Height;
    float UIScale;
};
