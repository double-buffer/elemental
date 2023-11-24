#include "NativeApplicationService.h"

// TODO: To remove
#include "SystemMemory.h"
#include "SystemFunctions.h"

HWND globalMainWindow;

DllExport void Native_InitNativeApplicationService(NativeApplicationOptions* options)
{
    if (options->LogMessageHandler)
    {
        globalLogMessageHandler = options->LogMessageHandler;
        LogDebugMessage(LogMessageCategory_NativeApplication, L"Init OK");
    }

    // TESTING
    auto memoryArena = SystemAllocateMemoryArena(1024);

    auto output = SystemExecuteProcess(memoryArena, "dir");
    printf("Allocated Bytes: %lld\n", SystemGetMemoryArenaAllocatedBytes(memoryArena));
    printf("Output: %s\n", output.Pointer);

    {
        auto stackMemoryArena1 = SystemGetStackMemoryArena();
        auto string1 = SystemConcatBuffers<char>(stackMemoryArena1, "Test", "Stack1");
        ReadOnlySpan<char> string2;
        ReadOnlySpan<char> string5;

        {
            auto stackMemoryArena2 = SystemGetStackMemoryArena();

            string2 = SystemConcatBuffers<char>(stackMemoryArena1, "Test2", "Stack1");
            auto string3 = SystemConcatBuffers<char>(stackMemoryArena2, "Test", "Stack2");
            printf("Stack2 String: %s\n", string3.Pointer);

            {
                auto stackMemoryArena3 = SystemGetStackMemoryArena();
                string5 = SystemConcatBuffers<char>(stackMemoryArena1, "Test4", "Stack1");
            }
        }

        auto string4 = SystemConcatBuffers<char>(stackMemoryArena1, "Test3", "Stack1");

        printf("Stack1 String: %s\n", string1.Pointer);
        printf("Stack1 String2: %s\n", string2.Pointer);
        printf("Stack1 String3: %s\n", string4.Pointer);
        printf("Stack1 String4: %s\n", string5.Pointer);
    }

    auto result = SystemConcatBuffers<wchar_t>(memoryArena, L"Test1 Wide ", L"Test2 Wide");
    printf("Result: %ls\n", result.Pointer);
    
    auto utf8 = SystemConvertWideCharToUtf8(memoryArena, result);
    printf("Result UTF8: %s\n", utf8.Pointer);
    printf("Result Back wchar: %ls\n", SystemConvertUtf8ToWideChar(memoryArena, utf8).Pointer);

    auto resultSplit = SystemSplitString<wchar_t>(memoryArena, result, L' ');

    for (size_t i = 0; i < resultSplit.Length; i++)
    {
        printf("ResultSplit: %ls\n", resultSplit[i].Pointer);
    }

    //SystemPopMemory(memoryArena, 2500);
    SystemClearMemoryArena(memoryArena);
    
    auto result2 = SystemConcatBuffers<char>(memoryArena, "Test1 ", "Test2");
    printf("Result: %s\n", result2.Pointer);

    auto resultSplit2 = SystemSplitString<char>(memoryArena, result2, ' ');

    for (size_t i = 0; i < resultSplit2.Length; i++)
    {
        printf("ResultSplit: %s\n", resultSplit2[i].Pointer);
    }

    printf("Allocated Bytes: %lld\n", SystemGetMemoryArenaAllocatedBytes(memoryArena));
}

DllExport void Native_FreeNativeApplicationService()
{
}

DllExport void* Native_CreateApplication(wchar_t* applicationName)
{
    auto application = new Win32Application();
    application->ApplicationInstance = (HINSTANCE)GetModuleHandle(nullptr);

    WNDCLASS windowClass {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32WindowCallBack;
	windowClass.hInstance = application->ApplicationInstance;
	windowClass.lpszClassName = L"ElementalWindowClass";
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));

    RegisterClass(&windowClass);

    return application;
}

DllExport void Native_FreeApplication(void* applicationPointer)
{
    delete (Win32Application*)applicationPointer;
}

DllExport void Native_RunApplication(Win32Application* application, RunHandlerPtr runHandler)
{
    auto canRun = true;

    while (canRun) 
    {
        ProcessMessages(application);
        canRun = runHandler(application->Status);

        if (application->IsStatusActive(NativeApplicationStatusFlags::Closing))
        {
            canRun = false;
        }
    }
}

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
    clientRectangle.right = (LONG)((float_t)width * mainScreenScaling);
    clientRectangle.bottom = (LONG)((float_t)height * mainScreenScaling);

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

    // HACK
    globalMainWindow = window;

    return nativeWindow;
}

DllExport void Native_FreeWindow(Win32Window* window)
{
    DestroyWindow(window->WindowHandle);
    delete window;
}

DllExport NativeWindowSize Native_GetWindowRenderSize(Win32Window* nativeWindow)
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

DllExport void Native_SetWindowTitle(Win32Window* nativeWindow, wchar_t* title)
{
    SetWindowText(nativeWindow->WindowHandle, title);
}
    
DllExport void Native_SetWindowState(Win32Window* window, NativeWindowState windowState)
{
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
}

void ProcessMessages(Win32Application* application)
{
    MSG message;

	while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
	{
        if (message.message == WM_QUIT)
        {
            application->SetStatus(NativeApplicationStatusFlags::Closing, 1);
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}

// TODO: Change that
static std::map<HWND, Win32Window*> windowMap = std::map<HWND, Win32Window*>();

LRESULT CALLBACK Win32WindowCallBack(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
    case WM_CREATE:
    {
        auto createParameters = (LPCREATESTRUCT)lParam;
        auto nativeWindow = (Win32Window*)createParameters->lpCreateParams;
        windowMap[window] = nativeWindow;
        break;
    }

    case WM_ACTIVATE:
	{
		//isAppActive = !(wParam == WA_INACTIVE);
		break;
	}

    case WM_MENUCHAR:
        return MAKELRESULT(0, MNC_CLOSE);

    case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN)
		{
			if ((HIWORD(lParam) & KF_ALTDOWN))
			{
                auto nativeWindow = windowMap[window];
                Native_SetWindowState(nativeWindow, NativeWindowState_FullScreen);
            }
		}
		break;

    case WM_DPICHANGED:
    {
        RECT* const prcNewWindow = (RECT*)lParam;
        SetWindowPos(window,
            NULL,
            prcNewWindow ->left,
            prcNewWindow ->top,
            prcNewWindow->right - prcNewWindow->left,
            prcNewWindow->bottom - prcNewWindow->top,
            SWP_NOZORDER | SWP_NOACTIVATE);

        break;
    }

	case WM_CLOSE:
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}
	}

	return DefWindowProc(window, message, wParam, lParam);
}
