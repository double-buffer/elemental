#include "Win32Window.h"
#include "Win32Application.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "Libs/Win32DarkMode/DarkMode.h"

SystemDataPool<Win32WindowData, Win32WindowDataFull> windowDataPool;

void InitWindowMemory()
{
    if (!windowDataPool.Storage)
    {
        windowDataPool = SystemCreateDataPool<Win32WindowData, Win32WindowDataFull>(ApplicationMemoryArena, 10);
    }
}

Win32WindowData* GetWin32WindowData(ElemWindow window)
{
    return SystemGetDataPoolItem(windowDataPool, window);
}

Win32WindowDataFull* GetWin32WindowDataFull(ElemWindow window)
{
    return SystemGetDataPoolItemFull(windowDataPool, window);
}

ElemAPI ElemWindow ElemCreateWindow(ElemApplication application, const ElemWindowOptions* options)
{
    InitWindowMemory();

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto width = 1280;
    auto height = 720;
    auto title = "Elemental Window";

    if (options != nullptr)
    {
        if (options->Title)
        {
            title = options->Title;
        }

        if (options->Width != 0)
        {
            width = options->Width;
        }

        if (options->Height != 0)
        {
            height = options->Height;
        }
    }

    auto applicationData = GetApplicationData(application);

    auto window = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        L"ElementalWindowClass",
        SystemConvertUtf8ToWideChar(stackMemoryArena, title).Pointer,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        applicationData->ApplicationInstance,
        nullptr); // TODO: Pass info to createwindow event
   
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    auto mainScreenDpi = GetDpiForWindow(window);
    auto mainScreenScaling = static_cast<float>(mainScreenDpi) / 96.0f;

    RECT clientRectangle;
    clientRectangle.left = 0;
    clientRectangle.top = 0;
    clientRectangle.right = (LONG)((float)width * mainScreenScaling);
    clientRectangle.bottom = (LONG)((float)height * mainScreenScaling);

    AdjustWindowRectExForDpi(&clientRectangle, WS_OVERLAPPEDWINDOW, false, 0, mainScreenDpi);

    width = (int32_t)(clientRectangle.right - clientRectangle.left);
    height = (int32_t)(clientRectangle.bottom - clientRectangle.top);

    // Compute the position of the window to center it 
    RECT desktopRectangle;
    GetClientRect(GetDesktopWindow(), &desktopRectangle);
    int32_t x = (int32_t)((desktopRectangle.right / 2) - (width / 2));
    int32_t y = (int32_t)((desktopRectangle.bottom / 2) - (height / 2));

    // Dark mode
    // TODO: Don't include the full library for this
    InitDarkMode();
    BOOL value = TRUE;
    
    if (ShouldAppUseDarkMode())
    {
        DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    }

    SetWindowPos(window, nullptr, x, y, width, height, 0);
    ShowWindow(window, SW_NORMAL);

    auto handle = SystemAddDataPoolItem(windowDataPool, {
        .WindowHandle = window
    }); 

    SystemAddDataPoolItemFull(windowDataPool, handle, {
        .WindowPlacement = {}
    });

    return handle;
}

ElemAPI void ElemFreeWindow(ElemWindow window)
{
    SystemRemoveDataPoolItem(windowDataPool, window);
}

ElemAPI ElemWindowSize ElemGetWindowRenderSize(ElemWindow window)
{
    auto windowData = GetWin32WindowData(window);
    SystemAssert(windowData);

    RECT windowRectangle;
	GetClientRect(windowData->WindowHandle, &windowRectangle);

    auto mainScreenDpi = GetDpiForWindow(windowData->WindowHandle);
    auto mainScreenScaling = static_cast<float>(mainScreenDpi) / 96.0f;

    auto width = (uint32_t)(windowRectangle.right - windowRectangle.left);
    auto height = (uint32_t)(windowRectangle.bottom - windowRectangle.top);
    auto uiScale = mainScreenScaling;

    return 
    {
        .Width = width,
        .Height = height,
        .UIScale = uiScale,
        .WindowState = ElemWindowState_Normal // TODO
    };
}

ElemAPI void ElemSetWindowTitle(ElemWindow window, const char* title)
{
    auto windowData = GetWin32WindowData(window);
    SystemAssert(windowData);

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto wideTitle = SystemConvertUtf8ToWideChar(stackMemoryArena, title);

    SetWindowText(windowData->WindowHandle, wideTitle.Pointer);
}

ElemAPI void ElemSetWindowState(ElemWindow window, ElemWindowState windowState)
{
}




/*
DllExport void* Native_CreateWindow(Win32Application* nativeApplication, NativeWindowOptions* options)
{
    // BUG: Random Memory bug and sometimes function is really slow
    auto width = (int32_t)options->Width;
    auto height = (int32_t)options->Height;

    auto nativeWindow = new Win32Window();
    
    auto window = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        L"ElementalWindowClass",
        options->Title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        nativeApplication->ApplicationInstance,
        nativeWindow);
   
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    auto mainScreenDpi = GetDpiForWindow(window);
    auto mainScreenScaling = static_cast<float>(mainScreenDpi) / 96.0f;

    RECT clientRectangle;
    clientRectangle.left = 0;
    clientRectangle.top = 0;
    clientRectangle.right = (LONG)((float)width * mainScreenScaling);
    clientRectangle.bottom = (LONG)((float)height * mainScreenScaling);

    AdjustWindowRectExForDpi(&clientRectangle, WS_OVERLAPPEDWINDOW, false, 0, mainScreenDpi);

    width = (int32_t)(clientRectangle.right - clientRectangle.left);
    height = (int32_t)(clientRectangle.bottom - clientRectangle.top);

    // Compute the position of the window to center it 
    RECT desktopRectangle;
    GetClientRect(GetDesktopWindow(), &desktopRectangle);
    int32_t x = (int32_t)((desktopRectangle.right / 2) - (width / 2));
    int32_t y = (int32_t)((desktopRectangle.bottom / 2) - (height / 2));

    // Dark mode
    // TODO: Don't include the full library for this
    InitDarkMode();
    BOOL value = TRUE;
    
    if (ShouldAppUseDarkMode())
    {
        DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    }

    SetWindowPos(window, nullptr, x, y, width, height, 0);
    ShowWindow(window, SW_NORMAL);

    WINDOWPLACEMENT windowPlacement;
    GetWindowPlacement(window, &windowPlacement);

    nativeWindow->WindowHandle = window;
    nativeWindow->Width = (uint32_t)width;
    nativeWindow->Height = (uint32_t)height;
    nativeWindow->UIScale = mainScreenScaling;
    nativeWindow->WindowPlacement = windowPlacement;

    Native_SetWindowState(nativeWindow, options->WindowState);

    return nativeWindow;
}

DllExport void Native_FreeWindow(Win32Window* window)
{
    DestroyWindow(window->WindowHandle);
    delete window;
}

DllExport NativeWindowSize Native_GetWindowRenderSize(void* nativeWindow)
{
    RECT windowRectangle;
	GetClientRect(nativeWindow->WindowHandle, &windowRectangle);

    auto mainScreenDpi = GetDpiForWindow(nativeWindow->WindowHandle);
    auto mainScreenScaling = static_cast<float>(mainScreenDpi) / 96.0f;

    nativeWindow->Width = (uint32_t)(windowRectangle.right - windowRectangle.left);
    nativeWindow->Height = (uint32_t)(windowRectangle.bottom - windowRectangle.top);
    nativeWindow->UIScale = mainScreenScaling;

    auto result = NativeWindowSize();
    result.Width = nativeWindow->Width;
    result.Height = nativeWindow->Height;
    result.UIScale = nativeWindow->UIScale;

    LONG windowStyle = GetWindowLong(nativeWindow->WindowHandle, GWL_STYLE);
    WINDOWPLACEMENT windowPlacement;
    GetWindowPlacement(nativeWindow->WindowHandle, &windowPlacement);
    
    MONITORINFO monitorInfos = { sizeof(monitorInfos) };
    GetMonitorInfo(MonitorFromWindow(nativeWindow->WindowHandle, MONITOR_DEFAULTTOPRIMARY), &monitorInfos);

    auto screenWidth = (uint32_t)(monitorInfos.rcMonitor.right - monitorInfos.rcMonitor.left);
    auto screenHeight = (uint32_t)(monitorInfos.rcMonitor.bottom - monitorInfos.rcMonitor.top);

    if (screenWidth == nativeWindow->Width && screenHeight == nativeWindow->Height)
    {
        result.WindowState = NativeWindowState_FullScreen;
    }
    
    else if (nativeWindow->Width == 0 && nativeWindow->Height == 0)
    {
        result.WindowState = NativeWindowState_Minimized;
    }
    
    else if (windowPlacement.showCmd == SW_MAXIMIZE)
    {
        result.WindowState = NativeWindowState_Maximized;
    }

    else if (windowStyle & WS_OVERLAPPEDWINDOW)
    {
        result.WindowState = NativeWindowState_Normal;
    }

    return result;
}

DllExport void Native_SetWindowTitle(Win32Window* nativeWindow, char* title)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto wideTitle = SystemConvertUtf8ToWideChar(stackMemoryArena, title);

    SetWindowText(nativeWindow->WindowHandle, wideTitle.Pointer);
}
    
DllExport void Native_SetWindowState(Win32Window* window, NativeWindowState windowState)
{
    // BUG: When we first maximize and then switch to fullscreen, we have borders
    // It is ok when we switch to fullscren from a normal window

    LONG windowStyle = GetWindowLong(window->WindowHandle, GWL_STYLE);

    if (windowState == NativeWindowState_FullScreen && (windowStyle & WS_OVERLAPPEDWINDOW))
    {
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(window->WindowHandle, &windowPlacement);
        window->WindowPlacement = windowPlacement;

        MONITORINFO monitorInfos = { sizeof(monitorInfos) };
        GetMonitorInfo(MonitorFromWindow(window->WindowHandle, MONITOR_DEFAULTTOPRIMARY), &monitorInfos);

        SetWindowLong(window->WindowHandle, GWL_STYLE, windowStyle & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(window->WindowHandle, HWND_TOP,
            monitorInfos.rcMonitor.left, monitorInfos.rcMonitor.top,
            monitorInfos.rcMonitor.right - monitorInfos.rcMonitor.left,
            monitorInfos.rcMonitor.bottom - monitorInfos.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        ShowWindow(window->WindowHandle, SW_MAXIMIZE);
    }

    else 
    {
        SetWindowLong(window->WindowHandle, GWL_STYLE, windowStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window->WindowHandle, &window->WindowPlacement);
        SetWindowPos(window->WindowHandle, NULL, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        if (windowState == NativeWindowState_Maximized)
        {
            ShowWindow(window->WindowHandle, SW_MAXIMIZE);
        }
        
        else if (windowState == NativeWindowState_Minimized)
        {
            ShowWindow(window->WindowHandle, SW_MINIMIZE);
        }
    }
}*/
