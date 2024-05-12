#include "MacOSApplication.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"
#include "SystemPlatformFunctions.h"

MemoryArena ApplicationMemoryArena;
bool ApplicationExited = false;

void InitApplicationMemory()
{
    if (ApplicationMemoryArena.Storage == nullptr)
    {
        ApplicationMemoryArena = SystemAllocateMemoryArena();

        SystemLogDebugMessage(ElemLogMessageCategory_Application, "Init OK");

        #ifdef _DEBUG
        SystemLogDebugMessage(ElemLogMessageCategory_Application, "Debug Mode");
        #endif
    }
}

void CreateApplicationMenu(ReadOnlySpan<char> applicationName)
{
    using NS::StringEncoding::UTF8StringEncoding;

    auto applicationNameString = NS::String::string(applicationName.Pointer, UTF8StringEncoding);

    auto mainMenu = NS::RetainPtr(NS::Menu::alloc()->init(MTLSTR("MainMenu")));
    auto applicationMenuItem = NS::RetainPtr(mainMenu->addItem(MTLSTR("ApplicationMenu"), nullptr, MTLSTR("")));

    auto applicationSubMenu = NS::RetainPtr(NS::Menu::alloc()->init(MTLSTR("Application")));
    applicationMenuItem->setSubmenu(applicationSubMenu.get());

    auto aboutSelector = NS::MenuItem::registerActionCallback("about", [](void*, SEL, const NS::Object* pSender)
    {
        NS::Application::sharedApplication()->orderFrontStandardAboutPanel(nullptr);
    });

    applicationSubMenu->addItem(NS::String::string("About ", UTF8StringEncoding)->stringByAppendingString(applicationNameString), aboutSelector, MTLSTR(""));
    applicationSubMenu->addItem(NS::MenuItem::separatorItem());
    
    auto serviceMenuItem = applicationSubMenu->addItem(MTLSTR("Service"), nullptr, MTLSTR(""));
    auto serviceMenu = NS::RetainPtr(NS::Menu::alloc()->init(MTLSTR("Service")));
    serviceMenuItem->setSubmenu(serviceMenu.get());

    applicationSubMenu->addItem(NS::MenuItem::separatorItem());
    
    auto hideSelector = NS::MenuItem::registerActionCallback("appHide", [](void*,SEL,const NS::Object* pSender)
    {
        NS::Application::sharedApplication()->hide(pSender);
    });

    applicationSubMenu->addItem(NS::String::string("Hide ", UTF8StringEncoding)->stringByAppendingString(applicationNameString), hideSelector, MTLSTR("h"));
    
    auto hideOthersSelector = NS::MenuItem::registerActionCallback("appHideOthers", [](void*,SEL,const NS::Object* pSender)
    {
        NS::Application::sharedApplication()->hideOtherApplications(pSender);
    });

    auto hideOthersMenuItem = applicationSubMenu->addItem(NS::String::string("Hide Others", UTF8StringEncoding), hideOthersSelector, MTLSTR("h"));
    hideOthersMenuItem->setKeyEquivalentModifierMask(NS::EventModifierFlagCommand | NS::EventModifierFlagOption);
    
    auto showAllSelector = NS::MenuItem::registerActionCallback("appShowall", [](void*,SEL,const NS::Object* pSender)
    {
        NS::Application::sharedApplication()->unhideAllApplications(pSender);
    });

    applicationSubMenu->addItem(NS::String::string("Show All", UTF8StringEncoding), showAllSelector, MTLSTR(""));
    applicationSubMenu->addItem(NS::MenuItem::separatorItem());

    auto quitSelector = NS::MenuItem::registerActionCallback("appQuit", [](void*,SEL,const NS::Object* pSender)
    {
        NS::Application::sharedApplication()->terminate(pSender);
    });

    applicationSubMenu->addItem(NS::String::string("Quit ", UTF8StringEncoding)->stringByAppendingString(applicationNameString), quitSelector, MTLSTR("q"));

    auto windowMenuItem = NS::RetainPtr(mainMenu->addItem(MTLSTR("WindowMenu"), nullptr, MTLSTR("")));
    auto windowSubMenu = NS::RetainPtr(NS::Menu::alloc()->init(MTLSTR("Window")));
    windowMenuItem->setSubmenu(windowSubMenu.get());
    
    // TODO: The 2 items here should be on the top
    auto minimizeSelector = NS::MenuItem::registerActionCallback("minimize", [](void*,SEL,const NS::Object* pSender)
    {
        NS::Application::sharedApplication()->mainWindow()->miniaturize(nullptr);
    });

    windowSubMenu->addItem(NS::String::string("Minimize", UTF8StringEncoding), minimizeSelector, MTLSTR("m"));
    
    auto zoomSelector = NS::MenuItem::registerActionCallback("zoom", [](void*,SEL,const NS::Object* pSender)
    {
        NS::Application::sharedApplication()->mainWindow()->zoom(nullptr);
    });

    windowSubMenu->addItem(NS::String::string("Zoom", UTF8StringEncoding), zoomSelector, MTLSTR(""));

    NS::Application::sharedApplication()->setMainMenu(mainMenu.get());
    NS::Application::sharedApplication()->setServicesMenu(serviceMenu.get());
    NS::Application::sharedApplication()->setWindowsMenu(windowSubMenu.get());
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
        .Platform = ElemPlatform_MacOS,
        .ApplicationPath = applicationPath.Pointer,
        .SupportMultiWindows = true
    };
}

ElemAPI int32_t ElemRunApplication(const ElemRunApplicationParameters* parameters)
{
    InitApplicationMemory();

    auto applicationDelegate = MacOSApplicationDelegate(parameters);

    auto sharedApplication = NS::Application::sharedApplication();
    sharedApplication->setDelegate(&applicationDelegate);
    sharedApplication->run();

    return 0;
}

ElemAPI void ElemExitApplication(int32_t exitCode)
{
    if (exitCode == 0)
    {
        NS::Application::sharedApplication()->terminate(nullptr);
    }
    else 
    {
        exit(exitCode);
    }
}

MacOSApplicationDelegate::MacOSApplicationDelegate(const ElemRunApplicationParameters* parameters)
{
    _runParameters = parameters;
}

void MacOSApplicationDelegate::applicationDidFinishLaunching(NS::Notification* pNotification)
{
    auto sharedApplication = NS::Application::sharedApplication();
    sharedApplication->setActivationPolicy(NS::ActivationPolicy::ActivationPolicyRegular);
    sharedApplication->activateIgnoringOtherApps(true);
    sharedApplication->finishLaunching();

    auto applicationName = "Elemental Application";

    if (_runParameters && _runParameters->ApplicationName)
    {
        applicationName = _runParameters->ApplicationName;
    }

    CreateApplicationMenu(applicationName);

    if (_runParameters->InitHandler)
    {
        _runParameters->InitHandler(_runParameters->Payload);
    }
}

bool MacOSApplicationDelegate::applicationShouldTerminateAfterLastWindowClosed(NS::Application* pSender)
{
    return true;
}

NS::TerminateReply MacOSApplicationDelegate::applicationShouldTerminate(NS::Application* pSender) 
{
    ApplicationExited = true;

    if (_runParameters && _runParameters->FreeHandler)
    {
        _runParameters->FreeHandler(_runParameters->Payload);
    }

    return NS::TerminateReplyTerminateNow;
}
