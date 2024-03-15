#include "MacOSApplication.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

MemoryArena ApplicationMemoryArena;
SystemDataPool<MacOSApplicationData, MacOSApplicationDataFull> applicationPool;

void InitApplicationMemory()
{
    if (ApplicationMemoryArena.Storage == nullptr)
    {
        ApplicationMemoryArena = SystemAllocateMemoryArena();
        applicationPool = SystemCreateDataPool<MacOSApplicationData, MacOSApplicationDataFull>(ApplicationMemoryArena, 10);

        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Init OK");

        #ifdef _DEBUG
        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Debug Mode");
        #endif
    }
}

MacOSApplicationData* GetMacOSApplicationData(ElemApplication application)
{
    return SystemGetDataPoolItem(applicationPool, application);
}

MacOSApplicationDataFull* GetMacOSApplicationDataFull(ElemApplication application)
{
    return SystemGetDataPoolItemFull(applicationPool, application);
}

void ProcessEvents(ElemApplication application) 
{
    NS::Event* rawEvent = nullptr; 

    // BUG: It seems we have a memory leak when we move the mouse inside the window 😟

    do 
    {
        auto applicationDataFull = GetMacOSApplicationDataFull(application);
        SystemAssert(applicationDataFull);

        rawEvent = NS::Application::sharedApplication()->nextEventMatchingMask(NS::EventMaskAny, 0, NS::DefaultRunLoopMode(), true);

        if (rawEvent)
        {
            NS::Application::sharedApplication()->sendEvent(rawEvent);
        }
    } while (rawEvent != nullptr);
}

void CreateApplicationMenu(ReadOnlySpan<char> applicationName)
{
    using NS::StringEncoding::UTF8StringEncoding;

    auto applicationNameString = NS::String::string(applicationName.Pointer, UTF8StringEncoding);

    auto mainMenu = NS::TransferPtr(NS::Menu::alloc()->init(MTLSTR("MainMenu")));
    auto applicationMenuItem = NS::TransferPtr(mainMenu->addItem(MTLSTR("ApplicationMenu"), nullptr, MTLSTR("")));

    auto applicationSubMenu = NS::TransferPtr(NS::Menu::alloc()->init(MTLSTR("Application")));
    applicationMenuItem->setSubmenu(applicationSubMenu.get());

    auto aboutSelector = NS::MenuItem::registerActionCallback("about", [](void*, SEL, const NS::Object* pSender)
    {
        NS::Application::sharedApplication()->orderFrontStandardAboutPanel(nullptr);
    });

    applicationSubMenu->addItem(NS::String::string("About ", UTF8StringEncoding)->stringByAppendingString(applicationNameString), aboutSelector, MTLSTR(""));
    applicationSubMenu->addItem(NS::MenuItem::separatorItem());
    
    auto serviceMenuItem = applicationSubMenu->addItem(MTLSTR("Service"), nullptr, MTLSTR(""));
    auto serviceMenu = NS::TransferPtr(NS::Menu::alloc()->init(MTLSTR("Service")));
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

    auto windowMenuItem = NS::TransferPtr(mainMenu->addItem(MTLSTR("WindowMenu"), nullptr, MTLSTR("")));
    auto windowSubMenu = NS::TransferPtr(NS::Menu::alloc()->init(MTLSTR("Window")));
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

ElemAPI ElemApplication ElemCreateApplication(const char* applicationName)
{
    InitApplicationMemory();

    MacOSApplicationData applicationData = {};
    auto handle = SystemAddDataPoolItem(applicationPool, applicationData); 

    MacOSApplicationDataFull applicationDataFull = {};
    applicationDataFull.ApplicationDelegate = new MacOSApplicationDelegate(handle);
    applicationDataFull.Status = ElemApplicationStatus_Active;
    SystemAddDataPoolItemFull(applicationPool, handle, applicationDataFull);
    
    NS::Application* pSharedApplication = NS::Application::sharedApplication();
    pSharedApplication->setDelegate(applicationDataFull.ApplicationDelegate);
    pSharedApplication->setActivationPolicy(NS::ActivationPolicy::ActivationPolicyRegular);
    pSharedApplication->activateIgnoringOtherApps(true);
    pSharedApplication->finishLaunching();

    CreateApplicationMenu(applicationName);

    return handle;
}

ElemAPI void ElemRunApplication(ElemApplication application, ElemRunHandlerPtr runHandler)
{
    auto canRun = true;

    while (canRun) 
    {
        auto autoreleasePool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

        ProcessEvents(application);
        auto applicationDataFull = GetMacOSApplicationDataFull(application);
        SystemAssert(applicationDataFull);
        canRun = runHandler(applicationDataFull->Status);

        if (applicationDataFull->Status == ElemApplicationStatus_Closing)
        {
            canRun = false;
        }
    }
}

ElemAPI void ElemFreeApplication(ElemApplication application)
{
    auto applicationDataFull = GetMacOSApplicationDataFull(application);
    SystemAssert(applicationDataFull);

    delete applicationDataFull->ApplicationDelegate;
    
    SystemRemoveDataPoolItem(applicationPool, application);
}

MacOSApplicationDelegate::MacOSApplicationDelegate(ElemApplication application)
{
    _application = application;
}

MacOSApplicationDelegate::~MacOSApplicationDelegate()
{
}

bool MacOSApplicationDelegate::applicationShouldTerminateAfterLastWindowClosed(NS::Application* pSender)
{
    return true;
}

NS::TerminateReply MacOSApplicationDelegate::applicationShouldTerminate(NS::Application* pSender) 
{
    auto applicationDataFull = GetMacOSApplicationDataFull(_application);
    applicationDataFull->Status = ElemApplicationStatus_Closing;

    return NS::TerminateReplyTerminateCancel;
}
