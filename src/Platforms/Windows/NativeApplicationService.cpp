#include "WindowsCommon.h"
#include "NativeApplicationService.h"

DllExport void* Native_CreateApplication(unsigned char* applicationName)
{
    auto application = new WindowsApplication();
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

DllExport void Native_DeleteApplication(void* applicationPointer)
{
    delete (WindowsApplication*)applicationPointer;
}

DllExport void Native_RunApplication(void* applicationPointer, RunHandlerPtr runHandler)
{
    auto application = (WindowsApplication*)applicationPointer;
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

DllExport void* Native_CreateWindow(void* applicationPointer, NativeWindowOptions options)
{
    InitCommonControls();
    auto nativeApplication = (WindowsApplication*)applicationPointer;

    auto width = options.Width;
    auto height = options.Height;

    auto window = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        L"ElementalWindowClass",
        ConvertUtf8ToWString(options.Title).c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        nativeApplication->ApplicationInstance,
        0);
   
    HMODULE shcoreLibrary = LoadLibrary(L"shcore.dll");
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    auto mainScreenDpi = GetDpiForWindow(window);
    auto mainScreenScaling = static_cast<float>(mainScreenDpi) / 96.0f;

    RECT clientRectangle;
    clientRectangle.left = 0;
    clientRectangle.top = 0;
    clientRectangle.right = static_cast<LONG>(width * mainScreenScaling);
    clientRectangle.bottom = static_cast<LONG>(height * mainScreenScaling);

    AdjustWindowRectExForDpi(&clientRectangle, WS_OVERLAPPEDWINDOW, false, 0, mainScreenDpi);

    width = clientRectangle.right - clientRectangle.left;
    height = clientRectangle.bottom - clientRectangle.top;

    // Compute the position of the window to center it 
    RECT desktopRectangle;
    GetClientRect(GetDesktopWindow(), &desktopRectangle);
    int x = (desktopRectangle.right / 2) - (width / 2);
    int y = (desktopRectangle.bottom / 2) - (height / 2);

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

    if (options.WindowState == NativeWindowState::Maximized)
    {
        ShowWindow(window, SW_MAXIMIZE);
    }

    auto nativeWindow = new WindowsWindow();
    nativeWindow->WindowHandle = window;
    nativeWindow->Width = width;
    nativeWindow->Height = height;
    nativeWindow->UIScale = mainScreenScaling;

    return nativeWindow;
}

DllExport void Native_DeleteWindow(void* windowPointer)
{
    auto window = (WindowsWindow*)windowPointer;
    DestroyWindow(window->WindowHandle);
    delete window;
}

DllExport NativeWindowSize Native_GetWindowRenderSize(void* windowPointer)
{
    auto nativeWindow = (WindowsWindow*)windowPointer;

    RECT windowRectangle;
	GetClientRect(nativeWindow->WindowHandle, &windowRectangle);

    auto mainScreenDpi = GetDpiForWindow(nativeWindow->WindowHandle);
    auto mainScreenScaling = static_cast<float>(mainScreenDpi) / 96.0f;

    nativeWindow->Width = windowRectangle.right - windowRectangle.left;
    nativeWindow->Height = windowRectangle.bottom - windowRectangle.top;
    nativeWindow->UIScale = mainScreenScaling;

    auto result = NativeWindowSize();
    result.Width = nativeWindow->Width;
    result.Height = nativeWindow->Height;
    result.UIScale = nativeWindow->UIScale;

    return result;
}

DllExport void Native_SetWindowTitle(void* windowPointer, unsigned char* title)
{
    auto nativeWindow = (WindowsWindow*)windowPointer;
    SetWindowText(nativeWindow->WindowHandle, ConvertUtf8ToWString(title).c_str());
}

void ProcessMessages(WindowsApplication* application)
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

LRESULT CALLBACK Win32WindowCallBack(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: Bind correct application
	WindowsEvent event
	{
		window,
		message,
		wParam,
		lParam
	};

	switch (message)
	{
	case WM_ACTIVATE:
	{
		//isAppActive = !(wParam == WA_INACTIVE);
		break;
	}

	case WM_SIZE:
	{
		//doChangeSize = true;
		// TODO: Handle minimized state
		break;
	}
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
	default:
		return DefWindowProc(window, message, wParam, lParam);
	}

	return 0;
}