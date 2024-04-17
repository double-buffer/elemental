#include "Win32Application.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

#define WIN32_MAX_RUNLOOP 10u

MemoryArena ApplicationMemoryArena;
Span<Win32RunLoopHandler> Win32RunLoopHandlers;
uint32_t Win32CurrentRunLoopIndex;

// TODO: Use Xaml Islands?
void InitWin32ApplicationMemory()
{
    if (ApplicationMemoryArena.Storage == nullptr)
    {
        ApplicationMemoryArena = SystemAllocateMemoryArena();
        Win32RunLoopHandlers = SystemPushArray<Win32RunLoopHandler>(ApplicationMemoryArena, WIN32_MAX_RUNLOOP);

        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Init OK");

        #ifdef _DEBUG
        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Debug Mode");
        #endif
    }
}

void AddWin32RunLoopHandler(Win32RunLoopHandlerPtr handler, ElemHandle handle)
{
    SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Add Runloop handler");
    Win32RunLoopHandlers[Win32CurrentRunLoopIndex++] = 
    {
        .Function = handler,
        .Handle = handle,
        .NextIndex = 0
    };
}

void RemoveWin32RunLoopHandler(Win32RunLoopHandlerPtr handler)
{
    // TODO:
}

bool ProcessWin32Messages()
{
    MSG message;

	while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
	{
        if (message.message == WM_QUIT)
        {
            return false;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return true;
}

ElemAPI void ElemConfigureLogHandler(ElemLogHandlerPtr logHandler)
{
    if (logHandler)
    {
        SystemRegisterLogHandler(logHandler);
    } 
}

ElemAPI int32_t ElemRunApplication(const ElemRunApplicationParameters* parameters)
{
    InitWin32ApplicationMemory();
    
    if (parameters->InitHandler)
    {
        parameters->InitHandler(parameters->Payload);
    }
    
    auto canRun = true;

    while (canRun) 
    {
        // TODO: Check this because it could potentially add latency?
        canRun = ProcessWin32Messages();

        if (canRun)
        {
            for (uint32_t i = 0; i < Win32CurrentRunLoopIndex; i++)
            {
                auto handler = Win32RunLoopHandlers[i];
                handler.Function(handler.Handle);
            }
        }
    }

    if (parameters->FreeHandler)
    {
        parameters->FreeHandler(parameters->Payload);
    }

    return 0;
}
