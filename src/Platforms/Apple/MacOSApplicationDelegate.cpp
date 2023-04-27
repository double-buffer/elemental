#include "PreCompiledHeader.h"
#include "Elemental.h"
#include "SystemFunctions.h"
#include "MacOSApplication.h"
#include "MacOSWindow.h"
#include "MacOSApplicationDelegate.h"

MacOSApplicationDelegate::MacOSApplicationDelegate(MacOSApplication* application)
{
    _application = application;
}

MacOSApplicationDelegate::~MacOSApplicationDelegate()
{
}

bool MacOSApplicationDelegate::applicationShouldTerminateAfterLastWindowClosed(NS::Application* pSender)
{
    printf("Window shoud close\n");
    // TODO: It doesn't seems to be working :(
    return true;
}

NS::TerminateReply MacOSApplicationDelegate::applicationShouldTerminate(NS::Application* pSender) 
{
    printf("Should Terminate\n");

    _application->SetStatus(Closing, 1);
    return NS::TerminateReplyTerminateCancel;
}