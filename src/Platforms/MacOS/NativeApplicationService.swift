import Cocoa
import NativeElemental

@_cdecl("Native_CreateApplication")
public func createApplication(applicationName: UnsafeMutablePointer<Int8>) -> UnsafeMutableRawPointer {
    var processSerialNumber = ProcessSerialNumber(highLongOfPSN: 0, lowLongOfPSN: UInt32(kCurrentProcess))
    TransformProcessType(&processSerialNumber, UInt32(kProcessTransformToForegroundApplication))

    NSApplication.shared.activate(ignoringOtherApps: true)
    NSApplication.shared.finishLaunching()
    
    buildMainMenu(applicationName: String(cString: applicationName))

    let application = MacOSApplication()
    return Unmanaged.passRetained(application).toOpaque()
}

@_cdecl("Native_RunApplication")
public func runApplication(applicationPointer: UnsafeRawPointer, runHandler: RunHandlerPtr) {
    let application = MacOSApplication.fromPointer(applicationPointer)

    var canRun = true

    autoreleasepool {
        while (canRun) {
            processEvents(application)
            canRun = runHandler(application.getStatus())
        }
    }
}

public func processEvents(_ application: MacOSApplication) {
    var rawEvent: NSEvent? = nil

    repeat {
        if (application.isStatusActive(Active)) {
            rawEvent = NSApplication.shared.nextEvent(matching: .any, until: nil, inMode: .default, dequeue: true)
        }
        
        guard let event = rawEvent else {
            application.setStatus(Active)
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