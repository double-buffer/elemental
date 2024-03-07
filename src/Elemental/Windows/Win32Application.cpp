#include "Win32Application.h"

#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

MemoryArena ApplicationMemoryArena;
SystemDataPool<Win32ApplicationData, Win32ApplicationDataFull> applicationPool;

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
