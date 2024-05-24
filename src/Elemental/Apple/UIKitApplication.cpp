#include "UIKitApplication.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"
#include "SystemPlatformFunctions.h"

MemoryArena ApplicationMemoryArena;

uint64_t ApplePerformanceCounterStart;
uint64_t ApplePerformanceCounterFrequencyInSeconds;

void InitApplicationMemory()
{
    if (ApplicationMemoryArena.Storage == nullptr)
    {
        ApplicationMemoryArena = SystemAllocateMemoryArena();

        SystemLogDebugMessage(ElemLogMessageCategory_Application, "Init OK");

        #ifdef _DEBUG
        SystemLogDebugMessage(ElemLogMessageCategory_Application, "Debug Mode");
        #endif

        ApplePerformanceCounterStart = SystemPlatformGetHighPerformanceCounter();
        ApplePerformanceCounterFrequencyInSeconds = SystemPlatformGetHighPerformanceCounterFrequencyInSeconds();
    }
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
        .Platform = ElemPlatform_iOS,
        .ApplicationPath = applicationPath.Pointer
    };
}

ElemAPI int32_t ElemRunApplication(const ElemRunApplicationParameters* parameters)
{
    InitApplicationMemory();

    auto applicationDelegate = UIKitApplicationDelegate(parameters);
    return UI::ApplicationMain(0, nullptr, &applicationDelegate);
}

ElemAPI void ElemExitApplication(int32_t exitCode)
{
    exit(exitCode);
}

UIKitApplicationDelegate::UIKitApplicationDelegate(const ElemRunApplicationParameters* parameters)
{
    _runParameters = parameters;
}

bool UIKitApplicationDelegate::applicationDidFinishLaunching(UI::Application *pApp, NS::Value *options)
{
    if (_runParameters->InitHandler)
    {
        _runParameters->InitHandler(_runParameters->Payload);
    }

    return true;
}

void UIKitApplicationDelegate::applicationWillTerminate(UI::Application *pApp)
{
    if (_runParameters && _runParameters->FreeHandler)
    {
        _runParameters->FreeHandler(_runParameters->Payload);
    }
}
