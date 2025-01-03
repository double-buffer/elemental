#include "Win32Application.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"
#include "SystemPlatformFunctions.h"

#define WIN32_MAX_RUNLOOP 10u

MemoryArena ApplicationMemoryArena;
Span<Win32RunLoopHandler> Win32RunLoopHandlers;
uint32_t Win32CurrentRunLoopIndex;

uint64_t Win32PerformanceCounterStart;
uint64_t Win32PerformanceCounterFrequencyInSeconds;

void InitWin32ApplicationMemory()
{
    if (ApplicationMemoryArena.Storage == nullptr)
    {
        ApplicationMemoryArena = SystemAllocateMemoryArena();
        Win32RunLoopHandlers = SystemPushArray<Win32RunLoopHandler>(ApplicationMemoryArena, WIN32_MAX_RUNLOOP);

        SystemLogDebugMessage(ElemLogMessageCategory_Application, "Init OK.");

        #ifdef _DEBUG
        SystemLogDebugMessage(ElemLogMessageCategory_Application, "Debug Mode.");
        #endif

        Win32PerformanceCounterStart = SystemPlatformGetHighPerformanceCounter();
        Win32PerformanceCounterFrequencyInSeconds = SystemPlatformGetHighPerformanceCounterFrequencyInSeconds();
    }
}

void AddWin32RunLoopHandler(Win32RunLoopHandlerPtr handler, ElemHandle handle)
{
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

ElemAPI ElemSystemInfo ElemGetSystemInfo()
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto executablePath = SystemPlatformGetExecutablePath(stackMemoryArena);
    auto environment = SystemPlatformGetEnvironment(stackMemoryArena);
                      
    auto lastIndex = SystemLastIndexOf(executablePath, environment->PathSeparator);
    SystemAssert(lastIndex != -1);

    auto applicationPath = SystemPushArray<char>(stackMemoryArena, lastIndex + 2);
    SystemCopyBuffer(applicationPath, executablePath.Slice(0, lastIndex + 1));

    return
    {
        .Platform = ElemPlatform_Windows,
        .ApplicationPath = applicationPath.Pointer,
        .SupportMultiWindows = true
    };
}

// TODO: Put that in system functions
ReadOnlySpan<char> FormatMemorySize(MemoryArena memoryArena, uint64_t bytes) 
{
    const char* suffixes[] = { "B", "KB", "MB", "GB", "TB" }; // Extend if more are needed
    double size = bytes;
    size_t i = 0;

    while (size >= 1024 && i < sizeof(suffixes)/sizeof(suffixes[0]) - 1) 
    {
        size /= 1024.0;
        i++;
    }

    auto result = SystemPushArray<char>(memoryArena, 50);
    snprintf(result.Pointer, result.Length, "%.2f %s", size, suffixes[i]);

    return result;
}

ElemAPI int32_t ElemRunApplication(const ElemRunApplicationParameters* parameters)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
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
        auto allocationInfos = SystemGetAllocationInfos();

        SystemLogDebugMessage(ElemLogMessageCategory_Application, "Allocated lib memory before releasing: %s/%s", 
                                FormatMemorySize(stackMemoryArena, allocationInfos.CommittedBytes).Pointer, 
                                FormatMemorySize(stackMemoryArena, allocationInfos.ReservedBytes).Pointer);

        parameters->FreeHandler(parameters->Payload);
    }

    return 0;
}

ElemAPI void ElemExitApplication(int32_t exitCode)
{
    PostQuitMessage(exitCode);
}

