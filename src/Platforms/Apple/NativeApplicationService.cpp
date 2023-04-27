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
        auto autoreleasePool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

        ProcessEvents(application);
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

    windowHandle->setTitle(NS::String::string((const char*)options->Title, NS::StringEncoding::UTF8StringEncoding));
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

DllExport void Native_SetWindowTitle(MacOSWindow* nativeWindow, uint8_t* title)
{
    nativeWindow->WindowHandle->setTitle(NS::String::string((const char*)title, NS::StringEncoding::UTF8StringEncoding));
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

    /*
    let window = MacOSWindow.fromPointer(windowPointer)
        if (window.window.isMiniaturized) {
            window.window.deminiaturize(nil) 
        } else if (contentViewSize.width == windowSize.width && contentViewSize.height == windowSize.height) {
            window.window.toggleFullScreen(nil) 
        } else if (window.window.isZoomed) {
            window.window.zoom(nil)
        }

        if (windowState == NativeWindowState_Maximized && !window.window.isZoomed) {
            window.window.zoom(nil)
        } else if (windowState == NativeWindowState_FullScreen) {
            window.window.toggleFullScreen(nil)
        } else if (windowState == NativeWindowState_Minimized) {
            window.window.miniaturize(nil)
        } 
        */
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