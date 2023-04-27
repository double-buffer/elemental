#include "PreCompiledHeader.h"
#include "Elemental.h"
#include "SystemFunctions.h"
#include "MacOSApplication.h"
#include "MacOSWindow.h"
#include "MacOSApplicationDelegate.h"

static void ProcessEvents(MacOSApplication* application);

DllExport void Native_InitNativeApplicationService()
{
    #ifdef _DEBUG
    SystemInitDebugAllocations();
    #endif
    
    NS::ProcessInfo::processInfo()->disableSuddenTermination();
}

DllExport void Native_FreeNativeApplicationService()
{
    #ifdef _DEBUG
    SystemCheckAllocations("Elemental");
    #endif
}

DllExport void Native_FreeNativePointer(void* nativePointer)
{
    free(nativePointer);
}

DllExport void* Native_CreateApplication(uint8_t* applicationName)
{
    // TODO: Is it really needed? 
    ProcessSerialNumber processSerialNumber = {0, kCurrentProcess};
    TransformProcessType(&processSerialNumber, kProcessTransformToForegroundApplication);

    auto application = new MacOSApplication();
    application->ApplicationDelegate = new MacOSApplicationDelegate(application);

    NS::Application* pSharedApplication = NS::Application::sharedApplication();
    pSharedApplication->setDelegate(application->ApplicationDelegate);
    pSharedApplication->setActivationPolicy(NS::ActivationPolicy::ActivationPolicyRegular);
    pSharedApplication->activateIgnoringOtherApps(true);
    pSharedApplication->finishLaunching();

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
        NS::AutoreleasePool* autoreleasePool = NS::AutoreleasePool::alloc()->init();
        ProcessEvents(application);
        canRun = runHandler(application->Status);

        if (application->IsStatusActive(Closing)) 
        {
            canRun = false;
        }

        autoreleasePool->release();
    }
}

DllExport void* Native_CreateWindow(MacOSApplication* nativeApplication, NativeWindowOptions* options)
{
    auto windowHandle = NS::Window::alloc()->init(
        (CGRect){ {0.0, 0.0}, { (double)options->Width, (double)options->Height} },
        NS::WindowStyleMaskClosable | NS::WindowStyleMaskTitled | NS::WindowStyleMaskMiniaturizable | NS::WindowStyleMaskResizable,
        NS::BackingStoreBuffered,
        false );

    windowHandle->setTitle(NS::String::string((const char*)options->Title, NS::StringEncoding::UTF8StringEncoding));
    windowHandle->center();
    windowHandle->makeKeyAndOrderFront(nullptr);

    auto window = new MacOSWindow();
    window->WindowHandle = windowHandle;

    return window;
}

DllExport void Native_FreeWindow(MacOSWindow* window)
{
    if (window->WindowHandle)
    {
        window->WindowHandle->release();
    }

    delete window;
}

DllExport NativeWindowSize Native_GetWindowRenderSize(MacOSWindow* nativeWindow)
{
    auto result = NativeWindowSize();
    return result;
}

DllExport void Native_SetWindowTitle(MacOSWindow* nativeWindow, uint8_t* title)
{
  
}
    
DllExport void Native_SetWindowState(MacOSWindow* window, NativeWindowState windowState)
{
    
}

static void ProcessEvents(MacOSApplication* application) 
{
    NS::Event* rawEvent = nullptr; 

    do 
    {
        if (application->IsStatusActive(Active)) 
        {
            rawEvent = NS::Application::sharedApplication()->nextEventMatchingMask(NS::EventMaskAny, 0, NS::DefaultRunLoopMode(), true);
        }
    
        if (rawEvent == nullptr)
        {
            application->SetStatus(Active, 1);
            return;
        }

        NS::Application::sharedApplication()->sendEvent(rawEvent);
    } while (rawEvent != nullptr);
}