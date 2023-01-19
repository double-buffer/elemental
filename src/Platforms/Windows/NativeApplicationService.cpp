#include "WindowsCommon.h"
#include "../Common/Elemental.h"
#include "NativeApplicationService.h"

DllExport void* Native_CreateApplication(unsigned char* applicationName)
{
    auto application = new WindowsApplication();
    application->ApplicationInstance = (HINSTANCE)GetModuleHandle(nullptr);

    return application;
}

DllExport void Native_RunApplication(void* applicationPointer, RunHandlerPtr runHandler)
{
    auto application = (WindowsApplication*)applicationPointer;
    auto canRun = true;

    while (canRun) 
    {
        //processEvents(application)
        canRun = runHandler(application->Status);
    }
}

void ProcessMessages()
{
    MSG message;

	while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
	{
        if (message.message == WM_QUIT)
        {
            //result.IsRunning = 0;
            //return result;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    //return result;
}