#pragma once
#include "MacOSApplication.h"

class MacOSApplicationDelegate : public NS::ApplicationDelegate
{
    public:
        MacOSApplicationDelegate(MacOSApplication* application);
        ~MacOSApplicationDelegate();

        bool applicationShouldTerminateAfterLastWindowClosed(NS::Application* pSender ) override;
        NS::TerminateReply applicationShouldTerminate(NS::Application* pSender) override;

    private:
        MacOSApplication* _application;
};