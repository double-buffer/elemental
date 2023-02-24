import Cocoa
import NativeElemental

@_cdecl("Native_FreeNativePointer")
public func freeNativePointer(pointer: UnsafeRawPointer) {
    pointer.deallocate()
}

@_cdecl("Native_CreateApplication")
public func createApplication(applicationName: UnsafeMutablePointer<Int8>) -> UnsafeMutableRawPointer? {
    var processSerialNumber = ProcessSerialNumber(highLongOfPSN: 0, lowLongOfPSN: UInt32(kCurrentProcess))
    TransformProcessType(&processSerialNumber, UInt32(kProcessTransformToForegroundApplication))
 
    let application = MacOSApplication()

    let delegate = MacOSAppDelegate(application)
    NSApplication.shared.delegate = delegate
    NSApplication.shared.activate(ignoringOtherApps: true)
    NSApplication.shared.finishLaunching()
    
    buildMainMenu(applicationName: String(cString: applicationName))

    return Unmanaged.passRetained(application).toOpaque()
}

@_cdecl("Native_FreeApplication")
public func freeApplication(applicationPointer: UnsafeRawPointer) {
    MacOSApplication.release(applicationPointer)
}

@_cdecl("Native_RunApplication")
public func runApplication(applicationPointer: UnsafeRawPointer, runHandler: RunHandlerPtr) {
    let application = MacOSApplication.fromPointer(applicationPointer)
    var canRun = true

    autoreleasepool {
        while (canRun) {
            processEvents(application)
            canRun = runHandler(application.getStatus())

            if (application.isStatusActive(Closing)) {
                canRun = false
            }
        }
    }
}

@_cdecl("Native_CreateWindow")
public func createWindow(application: UnsafeRawPointer, optionsPointer: UnsafePointer<NativeWindowOptions>) -> UnsafeMutableRawPointer? {
    let options = optionsPointer.pointee
    let width = options.Width
    let height = options.Height
    
    let window = NSWindow(contentRect: NSMakeRect(0, 0, CGFloat(width), CGFloat(height)), 
                            styleMask: [.resizable, .titled, .miniaturizable, .closable], 
                            backing: .buffered, 
                            defer: false)

    window.title = String(cString: options.Title);

    window.center()
    window.makeKeyAndOrderFront(nil)

    if (options.WindowState == NativeWindowState_Maximized) {
        window.setFrame(window.screen!.visibleFrame, display: true, animate: false)
    } else if (options.WindowState == NativeWindowState_FullScreen) {
        window.toggleFullScreen(nil)
    } else if (options.WindowState == NativeWindowState_Minimized) {
        window.miniaturize(nil)
    }

    let nativeWindow = MacOSWindow(window)
    return Unmanaged.passRetained(nativeWindow).toOpaque()
}

@_cdecl("Native_FreeWindow")
public func freeWindow(windowPointer: UnsafeRawPointer) {
    MacOSWindow.release(windowPointer)
}

@_cdecl("Native_GetWindowRenderSize")
public func getWindowRenderSize(_ windowPointer: UnsafeRawPointer) -> NativeWindowSize {
    let window = MacOSWindow.fromPointer(windowPointer)

    let contentView = window.window.contentView! as NSView
    let mainScreenScaling = window.window.screen!.backingScaleFactor
    
    let contentViewSize = window.window.contentView!.frame.size
    let windowSize = window.window.screen!.frame.size

    var size = contentView.frame.size
    size.width *= mainScreenScaling;
    size.height *= mainScreenScaling;
    
    var windowState = NativeWindowState_Normal
    
    if (window.window.isMiniaturized) {
        windowState = NativeWindowState_Minimized
    } else if (contentViewSize.width == windowSize.width && contentViewSize.height == windowSize.height) {
        windowState = NativeWindowState_FullScreen
    } else if (window.window.isZoomed) {
        windowState = NativeWindowState_Maximized
    }

    return NativeWindowSize(Width: Int32(size.width), Height: Int32(size.height), UIScale: Float(mainScreenScaling), WindowState: windowState)
}

@_cdecl("Native_SetWindowTitle")
public func setWindowTitle(windowPointer: UnsafeRawPointer, title: UnsafeMutablePointer<Int8>) {
    let window = MacOSWindow.fromPointer(windowPointer)
    window.window.title = String(cString: title)
}

@_cdecl("Native_SetWindowState")
public func setWindowState(windowPointer: UnsafeRawPointer, windowState: NativeWindowState) {
    let window = MacOSWindow.fromPointer(windowPointer)

    let contentViewSize = window.window.contentView!.frame.size
    let windowSize = window.window.screen!.frame.size

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
}

private func processEvents(_ application: MacOSApplication) {
    var rawEvent: NSEvent? = nil

    repeat {
        if (application.isStatusActive(Active)) {
            rawEvent = NSApplication.shared.nextEvent(matching: .any, until: nil, inMode: .default, dequeue: true)
        }
        
        guard let event = rawEvent else {
            application.setStatus(Active, 1)
            return
        }

        switch event.type {
        default:
            NSApplication.shared.sendEvent(event)
        }
    } while (rawEvent != nil)
}

private func buildMainMenu(applicationName: String) {
    let mainMenu = NSMenu(title: "MainMenu")
    
    let menuItem = mainMenu.addItem(withTitle: "ApplicationMenu", action: nil, keyEquivalent: "")
    let subMenu = NSMenu(title: "Application")
    mainMenu.setSubmenu(subMenu, for: menuItem)
    
    subMenu.addItem(withTitle: "About \(applicationName)", action: #selector(NSApplication.orderFrontStandardAboutPanel(_:)), keyEquivalent: "")
    subMenu.addItem(NSMenuItem.separator())

    let servicesMenuSub = subMenu.addItem(withTitle: "Services", action: nil, keyEquivalent: "")
    let servicesMenu = NSMenu(title:"Services")
    mainMenu.setSubmenu(servicesMenu, for: servicesMenuSub)
    NSApp.servicesMenu = servicesMenu
    subMenu.addItem(NSMenuItem.separator())
    
    var menuItemAdded = subMenu.addItem(withTitle: "Hide \(applicationName)", action:#selector(NSApplication.hide(_:)), keyEquivalent:"h")
    menuItemAdded.target = NSApp

    menuItemAdded = subMenu.addItem(withTitle: "Hide Others", action:#selector(NSApplication.hideOtherApplications(_:)), keyEquivalent:"h")
    menuItemAdded.keyEquivalentModifierMask = [.command, .option]
    menuItemAdded.target = NSApp

    menuItemAdded = subMenu.addItem(withTitle: "Show All", action:#selector(NSApplication.unhideAllApplications(_:)), keyEquivalent:"")
    menuItemAdded.target = NSApp

    subMenu.addItem(NSMenuItem.separator())
    subMenu.addItem(withTitle: "Quit", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")

    let windowMenuItem = mainMenu.addItem(withTitle: "Window", action: nil, keyEquivalent: "")
    let windowSubMenu = NSMenu(title: "Window")
    mainMenu.setSubmenu(windowSubMenu, for: windowMenuItem)

    windowSubMenu.addItem(withTitle: "Minimize", action: #selector(NSWindow.performMiniaturize(_:)), keyEquivalent: "m")
    windowSubMenu.addItem(withTitle: "Zoom", action: #selector(NSWindow.performZoom), keyEquivalent: "")
    
    NSApp.mainMenu = mainMenu
    NSApp.windowsMenu = windowSubMenu
}