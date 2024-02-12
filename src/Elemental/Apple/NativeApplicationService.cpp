#include "NativeApplicationService.h"
#include "MacOSWindow.h"
#include "MacOSApplicationDelegate.h"

#include "SystemLogging.h"
#include "SystemMemory.h"

#include "Elemental.h"

struct MacOSApplicationFull
{
    ElemApplicationStatus Status;
};

static MemoryArena applicationMemoryArena;
static MacOSApplicationFull macOSApplicationFull; // TODO: Replace by pool

void InitMemory()
{
    if (applicationMemoryArena.Storage == nullptr)
    {
        applicationMemoryArena = SystemAllocateMemoryArena();

        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Init OK");

        #ifdef _DEBUG
        SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Debug Mode");
        #endif
    }
}

void ProcessEvents(ElemApplication application) 
{
    NS::SharedPtr<NS::Event> rawEvent; 

    // BUG: It seems we have a memory leak when we move the mouse inside the window 😟
    // BUG: If we press a key like Q or A while the app is running and then quit the app it crash 😢

    do 
    {
        //if (application->IsStatusActive(Active)) 
        if (true) 
        {
            rawEvent = NS::RetainPtr(NS::Application::sharedApplication()->nextEventMatchingMask(NS::EventMaskAny, 0, NS::DefaultRunLoopMode(), true));
        }
    
        if (!rawEvent.get())
        {
            //application->SetStatus(Active, 1);
            return;
        }

        NS::Application::sharedApplication()->sendEvent(rawEvent.get());
    } while (rawEvent.get() != nullptr);
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
    InitMemory();
    
    NS::ProcessInfo::processInfo()->disableSuddenTermination();

    // TODO: Is it really needed? 
    ProcessSerialNumber processSerialNumber = {0, kCurrentProcess};
    TransformProcessType(&processSerialNumber, kProcessTransformToForegroundApplication);

    auto application = new MacOSApplication();
    application->ApplicationDelegate = new MacOSApplicationDelegate(application);

    NS::Application* pSharedApplication = NS::Application::sharedApplication();
    pSharedApplication->setDelegate(application->ApplicationDelegate);
    pSharedApplication->setActivationPolicy(NS::ActivationPolicy::ActivationPolicyRegular);

    // TODO: It is not needed it seems
    ProcessSerialNumber psn = { 0, kCurrentProcess }; 
	TransformProcessType(& psn, kProcessTransformToForegroundApplication);

    pSharedApplication->activateIgnoringOtherApps(true);
    pSharedApplication->finishLaunching();

    CreateApplicationMenu(applicationName);

    // TODO: Write pool system
    return 1;
}

ElemAPI void ElemRunApplication(ElemApplication application, ElemRunHandlerPtr runHandler)
{
    auto canRun = true;

    while (canRun) 
    {
        auto autoreleasePool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

        ProcessEvents(application);
        canRun = runHandler(macOSApplicationFull.Status);

        /*if (application->IsStatusActive(Closing)) 
        {
            canRun = false;
        }*/
    }
}

ElemAPI void ElemFreeApplication(ElemApplication application)
{
}


DllExport void Native_InitNativeApplicationService(NativeApplicationOptions* options)
{
    /*if (options->LogMessageHandler)
    {
        SystemRegisterLogMessageHandler(options->LogMessageHandler);
        SystemLogDebugMessage(LogMessageCategory_NativeApplication, "Init OK");

        #ifdef _DEBUG
        SystemLogDebugMessage(LogMessageCategory_NativeApplication, "Debug Mode");
        #endif
    }*/

    NS::ProcessInfo::processInfo()->disableSuddenTermination();
}

DllExport void Native_FreeNativeApplicationService()
{
}

DllExport void Native_FreeNativePointer(void* nativePointer)
{
    free(nativePointer);
}

DllExport void* Native_CreateApplication(char* applicationName)
{
    // TODO: Is it really needed? 
    ProcessSerialNumber processSerialNumber = {0, kCurrentProcess};
    TransformProcessType(&processSerialNumber, kProcessTransformToForegroundApplication);

    auto application = new MacOSApplication();
    application->ApplicationDelegate = new MacOSApplicationDelegate(application);

    NS::Application* pSharedApplication = NS::Application::sharedApplication();
    pSharedApplication->setDelegate(application->ApplicationDelegate);
    pSharedApplication->setActivationPolicy(NS::ActivationPolicy::ActivationPolicyRegular);

    // TODO: It is not needed it seems
    ProcessSerialNumber psn = { 0, kCurrentProcess }; 
	TransformProcessType(& psn, kProcessTransformToForegroundApplication);

    pSharedApplication->activateIgnoringOtherApps(true);
    pSharedApplication->finishLaunching();

    CreateApplicationMenu(applicationName);

    return application;
}

DllExport void Native_FreeApplication(MacOSApplication* application)
{
    if (application->ApplicationDelegate)
    {
        delete application->ApplicationDelegate;
    }

    delete application;
}

DllExport void Native_RunApplication(MacOSApplication* application, RunHandlerPtr runHandler)
{
    auto canRun = true;

    while (canRun) 
    {
        auto autoreleasePool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

        //ProcessEvents(application);
        canRun = runHandler(application->Status);

        if (application->IsStatusActive(Closing)) 
        {
            canRun = false;
        }
    }
}

DllExport void* Native_CreateWindow(MacOSApplication* nativeApplication, NativeWindowOptions* options)
{
    auto windowHandle = NS::TransferPtr(NS::Window::alloc()->init(
        (CGRect){ {0.0, 0.0}, { (double)options->Width, (double)options->Height} },
        NS::WindowStyleMaskClosable | NS::WindowStyleMaskTitled | NS::WindowStyleMaskMiniaturizable | NS::WindowStyleMaskResizable,
        NS::BackingStoreBuffered,
        false ));

    //SystemLogDebugMessage(LogMessageCategory_NativeApplication, "Title %s", options->Title);

    // BUG: String conversion is not working for now :(
    auto titleTest = "Test Title TEMP";

    windowHandle->setTitle(NS::String::string(titleTest, NS::StringEncoding::UTF8StringEncoding));
    //windowHandle->setTitle(NS::String::string((const char*)options->Title, NS::StringEncoding::UnicodeStringEncoding));
    windowHandle->center();
    windowHandle->makeKeyAndOrderFront(nullptr);

    auto window = new MacOSWindow();
    window->WindowHandle = windowHandle;
    
    Native_SetWindowState(window, options->WindowState);

    return window;
}

DllExport void Native_FreeWindow(MacOSWindow* window)
{
    delete window;
}

DllExport NativeWindowSize Native_GetWindowRenderSize(MacOSWindow* nativeWindow)
{
    auto result = NativeWindowSize();

    auto contentView = nativeWindow->WindowHandle->contentView();
    auto screen = nativeWindow->WindowHandle->screen();
    
    auto mainScreenScaling = screen ? screen->backingScaleFactor() : 1.0f;
    auto contentViewSize = contentView->frame().size;
    
    result.Width = (uint32_t)(contentViewSize.width * mainScreenScaling);
    result.Height = (uint32_t)(contentViewSize.height * mainScreenScaling);
    result.UIScale = mainScreenScaling;
    result.WindowState = NativeWindowState_Normal;

    if (screen)
    {
        auto screenSize = screen->frame().size;

        if (nativeWindow->WindowHandle->miniaturized())
        {
            result.WindowState = NativeWindowState_Minimized;
        }
        else if (contentViewSize.width == screenSize.width && contentViewSize.height == screenSize.height)
        {
            result.WindowState = NativeWindowState_FullScreen;
        }
        else if (nativeWindow->WindowHandle->zoomed())
        {
            result.WindowState = NativeWindowState_Maximized;
        }
    }

    return result;
}

DllExport void Native_SetWindowTitle(MacOSWindow* nativeWindow, wchar_t* title)
{
    // BUG: String conversion is not working for now :(
    //nativeWindow->WindowHandle->setTitle(NS::String::string((const char*)title, NS::StringEncoding::UTF16StringEncoding));
}
    
DllExport void Native_SetWindowState(MacOSWindow* window, NativeWindowState windowState)
{
    auto contentView = window->WindowHandle->contentView();
    auto screen = window->WindowHandle->screen();

    if (!contentView || !screen)
    {
        return;
    }
    
    auto contentViewSize = contentView->frame().size;
    auto screenSize = screen->frame().size;

    if (window->WindowHandle->miniaturized() && windowState != NativeWindowState_Minimized)
    {
        window->WindowHandle->deminiaturize(nullptr);
    }
    else if (contentViewSize.width == screenSize.width && contentViewSize.height == screenSize.height && windowState != NativeWindowState_FullScreen)
    {
        window->WindowHandle->toggleFullScreen(nullptr);
    }
    else if (window->WindowHandle->zoomed() && windowState != NativeWindowState_Maximized)
    {
        window->WindowHandle->zoom(nullptr);
    }

    if (windowState == NativeWindowState_Maximized && !window->WindowHandle->zoomed())
    {
        window->WindowHandle->zoom(nullptr);
    }
    else if (windowState == NativeWindowState_FullScreen && contentViewSize.width != screenSize.width && contentViewSize.height != screenSize.height)
    {
        window->WindowHandle->toggleFullScreen(nullptr);
    }
    else if (windowState == NativeWindowState_Minimized && !window->WindowHandle->miniaturized())
    {
        window->WindowHandle->miniaturize(nullptr);
    }
}

