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

extern uint64_t ApplePerformanceCounterStart;
extern uint64_t ApplePerformanceCounterFrequencyInSeconds;

extern MemoryArena ApplicationMemoryArena;
extern bool ApplicationExited;
