#pragma once
#include "WindowsCommon.h"
#include "Libs/Win32DarkMode/DarkMode.h"
#include "StringConverters.h"

struct WindowsApplication
{
    HINSTANCE ApplicationInstance;
    NativeApplicationStatus Status;

    WindowsApplication()
    {
        Status.Status = 0;
        SetStatus(NativeApplicationStatusFlags::Active, 1);
    }
    
    bool IsStatusActive(NativeApplicationStatusFlags flag) 
    {
        return (Status.Status & flag) != 0;
    }
    
    void SetStatus(NativeApplicationStatusFlags flag, int value) 
    {
        Status.Status |= value << (flag - 1);
    }
};

struct WindowsWindow
{
    HWND WindowHandle;
    int Width;
    int Height;
    float UIScale;
};

void ProcessMessages(WindowsApplication* application);
LRESULT CALLBACK Win32WindowCallBack(HWND window, UINT message, WPARAM wParam, LPARAM lParam);