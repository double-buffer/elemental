#include "WaylandApplication.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

#define WIN32_MAX_RUNLOOP 10u

MemoryArena ApplicationMemoryArena;

void InitLinuxApplicationMemory()
{
    if (ApplicationMemoryArena.Storage == nullptr)
    {
        ApplicationMemoryArena = SystemAllocateMemoryArena();

        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Init OK.");

        #ifdef _DEBUG
        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Debug Mode.");
        #endif
    }
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
    InitLinuxApplicationMemory();
    
    if (parameters->InitHandler)
    {
        parameters->InitHandler(parameters->Payload);
    }
   /* 
    auto canRun = true;

    while (canRun) 
    {
        // TODO: Check this because it could potentially add latency?
        canRun = ProcessLinuxMessages();

        if (canRun)
        {
            for (uint32_t i = 0; i < LinuxCurrentRunLoopIndex; i++)
            {
                auto handler = LinuxRunLoopHandlers[i];
                handler.Function(handler.Handle);
            }
        }
    }*/

    if (parameters->FreeHandler)
    {
        parameters->FreeHandler(parameters->Payload);
    }

    return 0;
}
