#include "PreCompiledHeader.h"
#include "Elemental.h"
#include "SystemFunctions.h"

DllExport void Native_InitNativeApplicationService()
{
    printf("Hello Mac!\n");
    #ifdef _DEBUG
    SystemInitDebugAllocations();
    #endif
}

DllExport void Native_FreeNativeApplicationService()
{
    #ifdef _DEBUG
    SystemCheckAllocations("Elemental");
    #endif
}

/*
DllExport void* Native_CreateApplication(uint8_t* applicationName)
{
    return nullptr;
}

DllExport void Native_FreeApplication(void* applicationPointer)
{
}

DllExport void Native_RunApplication(Win32Application* application, RunHandlerPtr runHandler)
{
  
}

DllExport void* Native_CreateWindow(Win32Application* nativeApplication, NativeWindowOptions options)
{
    return nullptr;
}

DllExport void Native_FreeWindow(Win32Window* window)
{
}

DllExport NativeWindowSize Native_GetWindowRenderSize(Win32Window* nativeWindow)
{
    auto result = NativeWindowSize();
    return result;
}

DllExport void Native_SetWindowTitle(Win32Window* nativeWindow, uint8_t* title)
{
  
}
    
DllExport void Native_SetWindowState(Win32Window* window, NativeWindowState windowState)
{
    
}
*/