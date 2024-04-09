#include "UIKitApplication.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

MemoryArena ApplicationMemoryArena;

void InitApplicationMemory()
{
    if (ApplicationMemoryArena.Storage == nullptr)
    {
        ApplicationMemoryArena = SystemAllocateMemoryArena();

        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Init OK");

        #ifdef _DEBUG
        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Debug Mode");
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

ElemAPI int32_t ElemRunApplication2(const ElemRunApplicationParameters* parameters)
{
    InitApplicationMemory();

    auto applicationDelegate = UIKitApplicationDelegate(parameters);
    return UI::ApplicationMain(0, nullptr, &applicationDelegate);
}

ElemAPI void ElemFreeApplication(ElemApplication application)
{
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
