#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

class MacOSApplicationDelegate : public NS::ApplicationDelegate
{
    public:
        MacOSApplicationDelegate(const ElemRunApplicationParameters* runParameters);

    // TODO: It seems we need to create the app menu here...
	//	virtual void			applicationWillFinishLaunching( Notification* pNotification ) { }
        void applicationDidFinishLaunching(NS::Notification* pNotification) override;
        bool applicationShouldTerminateAfterLastWindowClosed(NS::Application* pSender ) override;
        NS::TerminateReply applicationShouldTerminate(NS::Application* pSender) override;

    private:
        const ElemRunApplicationParameters* _runParameters;
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
