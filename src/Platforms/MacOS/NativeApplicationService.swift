import Cocoa
import NativeElemental

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

@_cdecl("Native_DeleteApplication")
public func deleteApplication(applicationPointer: UnsafeRawPointer) {
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
public func createWindow(application: UnsafeRawPointer, description: NativeWindowDescription) -> UnsafeMutableRawPointer? {
    let width = description.Width
    let height = description.Height
    
    let window = NSWindow(contentRect: NSMakeRect(0, 0, CGFloat(width), CGFloat(height)), 
                            styleMask: [.resizable, .titled, .miniaturizable, .closable], 
                            backing: .buffered, 
                            defer: false)

    window.title = String(cString: description.Title);

    window.center()
    window.makeKeyAndOrderFront(nil)

    if (description.WindowState == Maximized) {
        window.setFrame(window.screen!.visibleFrame, display: true, animate: false)
    }

    let nativeWindow = MacOSWindow(window)
    return Unmanaged.passRetained(nativeWindow).toOpaque()
}

@_cdecl("Native_DeleteWindow")
public func deleteWindow(windowPointer: UnsafeRawPointer) {
    MacOSWindow.release(windowPointer)
}

@_cdecl("Native_GetWindowRenderSize")
public func getWindowRenderSize(windowPointer: UnsafeRawPointer) -> NativeWindowSize {
    let window = MacOSWindow.fromPointer(windowPointer)

    let contentView = window.window.contentView! as NSView
    let mainScreenScaling = window.window.screen!.backingScaleFactor

    var size = contentView.frame.size
    size.width *= mainScreenScaling;
    size.height *= mainScreenScaling;

    return NativeWindowSize(Width: Int32(size.width), Height: Int32(size.height), UIScale: Float(mainScreenScaling))
}

@_cdecl("Native_SetWindowTitle")
public func setWindowTitle(windowPointer: UnsafeRawPointer, title: UnsafeMutablePointer<Int8>) {
    let window = MacOSWindow.fromPointer(windowPointer)
    window.window.title = String(cString: title)
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