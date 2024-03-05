#include "Win32Application.h"

#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

MemoryArena ApplicationMemoryArena;
SystemDataPool<Win32ApplicationData, Win32ApplicationDataFull> applicationPool;

// TODO: IMPORTANT: Move common code to his own project?

// TODO: OLD CODE
//static SystemDictionary<HWND, WindowsWindow> windowMap;

// TODO: Rename
void InitApplicationMemory()
{
    if (ApplicationMemoryArena.Storage == nullptr)
    {
        ApplicationMemoryArena = SystemAllocateMemoryArena();
        applicationPool = SystemCreateDataPool<Win32ApplicationData, Win32ApplicationDataFull>(ApplicationMemoryArena, 10);

        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Init OK");

        #ifdef _DEBUG
        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Debug Mode");
        #endif
    }
}

Win32ApplicationData* GetApplicationData(ElemApplication application)
{
    return SystemGetDataPoolItem(applicationPool, application);
}

Win32ApplicationDataFull* GetApplicationDataFull(ElemApplication application)
{
    return SystemGetDataPoolItemFull(applicationPool, application);
}

void ProcessMessages(ElemApplication application)
{
    MSG message;

	while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
	{
        if (message.message == WM_QUIT)
        {
            auto applicationDataFull = GetApplicationDataFull(application);
            SystemAssert(applicationDataFull);
            applicationDataFull->Status = ElemApplicationStatus_Closing;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}

LRESULT CALLBACK WindowCallBack(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
    case WM_CREATE:
    {
        //auto createParameters = (LPCREATESTRUCT)lParam;
        // TODO: 
        //auto nativeWindow = (Win32Window*)createParameters->lpCreateParams;

        //SystemAddDictionaryEntry(windowMap, window, nativeWindow);
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
                // TODO:
                //auto nativeWindow = windowMap[window];
                //Native_SetWindowState(nativeWindow, NativeWindowState_FullScreen);
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

ElemAPI void ElemConfigureLogHandler(ElemLogHandlerPtr logHandler)
{
    if (logHandler)
    {
        SystemRegisterLogHandler(logHandler);
    } 
}

ElemAPI ElemApplication ElemCreateApplication(const char* applicationName)
{
    InitApplicationMemory();

    auto instance = (HINSTANCE)GetModuleHandle(nullptr);

    WNDCLASS windowClass {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowCallBack;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = L"ElementalWindowClass";
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));

    RegisterClass(&windowClass);

    auto handle = SystemAddDataPoolItem(applicationPool, {
        .ApplicationInstance = instance
    }); 

    SystemAddDataPoolItemFull(applicationPool, handle, {
        .Status = ElemApplicationStatus_Active
    });

    return handle;
}

ElemAPI void ElemFreeApplication(ElemApplication application)
{
    SystemRemoveDataPoolItem(applicationPool, application);
}

ElemAPI void ElemRunApplication(ElemApplication application, ElemRunHandlerPtr runHandler)
{
    auto canRun = true;

    while (canRun) 
    {
        ProcessMessages(application);

        auto applicationDataFull = GetApplicationDataFull(application);
        SystemAssert(applicationDataFull);

        canRun = runHandler(applicationDataFull->Status);

        if (applicationDataFull->Status == ElemApplicationStatus_Closing)
        {
            canRun = false;
        }
    }
}
