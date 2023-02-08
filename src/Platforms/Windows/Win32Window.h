#pragma once
#include "WindowsCommon.h"

struct Win32Window
{
    HWND WindowHandle;
    int Width;
    int Height;
    float UIScale;
};