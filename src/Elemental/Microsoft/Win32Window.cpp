#include "Win32Window.h"
#include "Win32Application.h"
#include "Win32Inputs.h"
#include "SystemDataPool.h"
#include "SystemDictionary.h"
#include "SystemFunctions.h"

SystemDataPool<Win32WindowData, Win32WindowDataFull> windowDataPool;
SystemDictionary<HWND, ElemWindow> windowDictionary;

void RefreshWin32MonitorInfos(ElemWindow window)
{
    auto windowData = GetWin32WindowData(window);
    SystemAssert(windowData);

    auto windowDataFull = GetWin32WindowDataFull(window);
    SystemAssert(windowDataFull);

    SystemLogDebugMessage(ElemLogMessageCategory_Application, "Refreshing monitor infos...");

    ComPtr<IDXGIFactory2> dxgiFactory = nullptr;
    AssertIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

    ComPtr<IDXGIAdapter> adapter = nullptr;

    for (uint32_t i = 0; dxgiFactory->EnumAdapters(i, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; i++)
    {
        ComPtr<IDXGIOutput> output = nullptr;

        for (uint32_t j = 0; adapter->EnumOutputs(j, output.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; j++)
        {
            DXGI_OUTPUT_DESC outputDescription;
            output->GetDesc(&outputDescription);

            if (outputDescription.Monitor == windowDataFull->Monitor) 
            {
                DXGI_MODE_DESC currentMode
                {
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM
                };
                
                if (SUCCEEDED(output->FindClosestMatchingMode(&currentMode, &currentMode, nullptr))) 
                {
                    uint32_t refreshRate = SystemRoundUp((float)currentMode.RefreshRate.Numerator / currentMode.RefreshRate.Denominator);
                    windowData->MonitorRefreshRate = refreshRate;
                }
                break;
            }
        }
    }
}

LRESULT CALLBACK WindowCallBack(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
        case WM_MENUCHAR:
        {
            return MAKELRESULT(0, MNC_CLOSE);
        }

        // TODO: Don't use the windows events for inputs
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            if (!SystemDictionaryContainsKey(windowDictionary, window))
            {
                break;
            }

            ProcessWin32KeyInput(windowDictionary[window], message, wParam, lParam);
            break;
        }

        case WM_INPUT:
        {
            if (!SystemDictionaryContainsKey(windowDictionary, window))
            {
                break;
            }

            ProcessWin32RawInput(windowDictionary[window], lParam);
            break;
        };
        
        /*
        case WM_MOUSEMOVE:
        {
            if (!SystemDictionaryContainsKey(windowDictionary, window))
            {
                break;
            }

            ProcessWin32MouseMove(windowDictionary[window], message, wParam, lParam);
            break;
        }*/

        case WM_SYSKEYDOWN:
        {
            if (wParam == VK_RETURN)
            {
                if ((HIWORD(lParam) & KF_ALTDOWN))
                {
                    if (!SystemDictionaryContainsKey(windowDictionary, window))
                    {
                        SystemLogErrorMessage(ElemLogMessageCategory_Application, "Cannot enter fullscreen because window is not valid.");
                        break;
                    }

                    auto windowHandle = windowDictionary[window];
                    ElemSetWindowState(windowHandle, ElemWindowState_FullScreen);
                }
            }
            break;
        }

        case WM_DPICHANGED:
        {
            auto newSize = (RECT*)lParam;

            SetWindowPos(window, nullptr,
                newSize ->left,
                newSize ->top,
                newSize->right - newSize->left,
                newSize->bottom - newSize->top,
                SWP_NOZORDER | SWP_NOACTIVATE);

            break;
        }

        case WM_DISPLAYCHANGE:
        case WM_WINDOWPOSCHANGED:
        {
            if (!SystemDictionaryContainsKey(windowDictionary, window))
            {
                break;
            }

            auto windowHandle = windowDictionary[window];

            auto windowData = GetWin32WindowData(windowHandle);
            SystemAssert(windowData);

            auto windowDataFull = GetWin32WindowDataFull(windowHandle);
            SystemAssert(windowDataFull);

            auto currentMonitor = MonitorFromWindow(windowData->WindowHandle, MONITOR_DEFAULTTONEAREST);

            if (currentMonitor != windowDataFull->Monitor)
            {
                windowDataFull->Monitor = currentMonitor;
                RefreshWin32MonitorInfos(windowHandle);
            }
            break;
        }
        case WM_CLOSE:
        {
            if (!SystemDictionaryContainsKey(windowDictionary, window))
            {
                SystemLogErrorMessage(ElemLogMessageCategory_Application, "Cannot destroy window because the window is not known.");
            }

            auto windowHandle = windowDictionary[window];
            ElemFreeWindow(windowHandle);
            PostQuitMessage(0);
            break;
        }
	}

	return DefWindowProc(window, message, wParam, lParam);
}

bool ShouldAppUseDarkMode()
{
    auto uxthemeLibrary = LoadLibraryEx(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

    if (!uxthemeLibrary)
    {
        return false;
    }

    auto shouldAppsUseDarkModeFunction = (bool (WINAPI *)())GetProcAddress(uxthemeLibrary, MAKEINTRESOURCEA(132));

    if (!shouldAppsUseDarkModeFunction)
    {
        return false;
    }

	return shouldAppsUseDarkModeFunction();
}

void InitWin32WindowMemory()
{
    if (!windowDataPool.Storage)
    {
        windowDataPool = SystemCreateDataPool<Win32WindowData, Win32WindowDataFull>(ApplicationMemoryArena, 10);
        windowDictionary = SystemCreateDictionary<HWND, ElemWindow>(ApplicationMemoryArena, 10);

        WNDCLASS windowClass {};
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowCallBack;
        windowClass.hInstance = GetModuleHandle(nullptr);
        windowClass.lpszClassName = L"ElementalWindowClass";
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));

        RegisterClass(&windowClass);
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

ElemAPI ElemWindow ElemCreateWindow(const ElemWindowOptions* options)
{
    InitWin32WindowMemory();

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto width = 1280;
    auto height = 720;
    auto title = "Elemental Window";
    auto windowState = ElemWindowState_Normal;

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

        if (options->WindowState != 0)
        {
            windowState = options->WindowState;
        }
    }

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
        GetModuleHandle(nullptr),
        nullptr);
   
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    auto mainScreenDpi = GetDpiForWindow(window);
    auto mainScreenScaling = static_cast<float>(mainScreenDpi) / 96.0f;

    RECT clientRectangle
    {
        .left = 0,
        .top = 0,
        .right = (LONG)((float)width * mainScreenScaling),
        .bottom = (LONG)((float)height * mainScreenScaling)
    };

    AdjustWindowRectExForDpi(&clientRectangle, WS_OVERLAPPEDWINDOW, false, 0, mainScreenDpi);
    width = (int32_t)(clientRectangle.right - clientRectangle.left);
    height = (int32_t)(clientRectangle.bottom - clientRectangle.top);

    RECT desktopRectangle;
    GetClientRect(GetDesktopWindow(), &desktopRectangle);
    auto x = (int32_t)(desktopRectangle.right / 2 - width / 2);
    auto y = (int32_t)(desktopRectangle.bottom / 2 - height / 2);

    if (ShouldAppUseDarkMode())
    {
        auto enableDarkMode = 1u;
        DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &enableDarkMode, sizeof(enableDarkMode));
    }

    SetWindowPos(window, nullptr, x, y, width, height, 0);
    ShowWindow(window, SW_SHOWNORMAL);

    WINDOWPLACEMENT windowPlacement;
    GetWindowPlacement(window, &windowPlacement);

    DWORD windowStyle = GetWindowLong(window, GWL_STYLE);
    DWORD windowExStyle = GetWindowLong(window, GWL_EXSTYLE);

    auto currentMonitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);

    auto handle = SystemAddDataPoolItem(windowDataPool, {
        .WindowHandle = window
    }); 

    SystemAddDataPoolItemFull(windowDataPool, handle, {
        .WindowPlacement = windowPlacement,
        .WindowStyle = windowStyle,
        .WindowExStyle = windowExStyle,
        .Monitor = currentMonitor
    });

    SystemAddDictionaryEntry(windowDictionary, window, handle);

    ElemSetWindowState(handle, windowState);
    RefreshWin32MonitorInfos(handle);

    InitWin32Inputs(window);

    return handle;
}

ElemAPI void ElemFreeWindow(ElemWindow window)
{
    auto windowData = GetWin32WindowData(window);

    if (windowData == ELEM_HANDLE_NULL)
    {
        return;
    }

    auto windowDataFull = GetWin32WindowDataFull(window);
    SystemAssert(windowDataFull);

    // TODO: Close application if last window

    DestroyWindow(windowData->WindowHandle);

    SystemRemoveDictionaryEntry(windowDictionary, windowData->WindowHandle);
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
    auto windowState = ElemWindowState_Normal;

    WINDOWPLACEMENT windowPlacement;
    GetWindowPlacement(windowData->WindowHandle, &windowPlacement);
    
    MONITORINFO monitorInfos = { sizeof(monitorInfos) };
    GetMonitorInfo(MonitorFromWindow(windowData->WindowHandle, MONITOR_DEFAULTTOPRIMARY), &monitorInfos);

    auto screenWidth = (uint32_t)(monitorInfos.rcMonitor.right - monitorInfos.rcMonitor.left);
    auto screenHeight = (uint32_t)(monitorInfos.rcMonitor.bottom - monitorInfos.rcMonitor.top);

    if (screenWidth == width && screenHeight == height)
    {
        windowState = ElemWindowState_FullScreen;
    }
    else if (windowPlacement.showCmd == SW_MINIMIZE || windowPlacement.showCmd == SW_SHOWMINIMIZED)
    {
        windowState = ElemWindowState_Minimized;
    }
    else if (windowPlacement.showCmd == SW_MAXIMIZE || windowPlacement.showCmd == SW_SHOWMAXIMIZED)
    {
        windowState = ElemWindowState_Maximized;
    }

    return 
    {
        .Width = width,
        .Height = height,
        .UIScale = uiScale,
        .WindowState = windowState
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
    auto windowData = GetWin32WindowData(window);
    SystemAssert(windowData);

    auto windowDataFull = GetWin32WindowDataFull(window);
    SystemAssert(windowDataFull);

    DWORD windowStyle = GetWindowLong(windowData->WindowHandle, GWL_STYLE);

    if (windowState == ElemWindowState_FullScreen && (windowStyle & WS_OVERLAPPEDWINDOW))
    {
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(windowData->WindowHandle, &windowPlacement);

        DWORD windowExStyle = GetWindowLong(windowData->WindowHandle, GWL_EXSTYLE);

        windowDataFull->WindowPlacement = windowPlacement;
        windowDataFull->WindowStyle = windowStyle;
        windowDataFull->WindowExStyle = windowExStyle;

        MONITORINFO monitorInfos = { sizeof(monitorInfos) };
        GetMonitorInfo(MonitorFromWindow(windowData->WindowHandle, MONITOR_DEFAULTTOPRIMARY), &monitorInfos);

        SetWindowLong(windowData->WindowHandle, GWL_STYLE, WS_VISIBLE);
        SetWindowLong(windowData->WindowHandle, GWL_EXSTYLE, WS_EX_APPWINDOW);

        SetWindowPos(windowData->WindowHandle, HWND_TOP,
            monitorInfos.rcMonitor.left, monitorInfos.rcMonitor.top,
            monitorInfos.rcMonitor.right - monitorInfos.rcMonitor.left,
            monitorInfos.rcMonitor.bottom - monitorInfos.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        ShowWindow(windowData->WindowHandle, SW_SHOWMAXIMIZED);
    }
    else 
    {
        SetWindowLong(windowData->WindowHandle, GWL_STYLE, windowDataFull->WindowStyle);
        SetWindowLong(windowData->WindowHandle, GWL_EXSTYLE, windowDataFull->WindowExStyle);

        SetWindowPlacement(windowData->WindowHandle, &windowDataFull->WindowPlacement);

        if (windowState == ElemWindowState_Maximized)
        {
            ShowWindow(windowData->WindowHandle, SW_SHOWMAXIMIZED);
        }

        else if (windowState == ElemWindowState_Minimized)
        {
            ShowWindow(windowData->WindowHandle, SW_SHOWMINIMIZED);
        }
    }
}
