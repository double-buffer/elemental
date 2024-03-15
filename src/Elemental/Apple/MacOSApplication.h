#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

class MacOSApplicationDelegate : public NS::ApplicationDelegate
{
    public:
        MacOSApplicationDelegate(ElemApplication application);
        ~MacOSApplicationDelegate();

        bool applicationShouldTerminateAfterLastWindowClosed(NS::Application* pSender ) override;
        NS::TerminateReply applicationShouldTerminate(NS::Application* pSender) override;

    private:
        ElemApplication _application;
};

struct MacOSApplicationData
{
};

struct MacOSApplicationDataFull
{
    MacOSApplicationDelegate* ApplicationDelegate;
    ElemApplicationStatus Status;
    int32_t WindowCount;
};

extern MemoryArena ApplicationMemoryArena;

MacOSApplicationData* GetMacOSApplicationData(ElemApplication application);
MacOSApplicationDataFull* GetMacOSApplicationDataFull(ElemApplication application);
