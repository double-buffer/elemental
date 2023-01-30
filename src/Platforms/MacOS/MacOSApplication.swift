import NativeElemental
import Cocoa

public class MacOSApplication {
    private var status: NativeApplicationStatus

    public init() {
        status = NativeApplicationStatus()
        setStatus(Active, 1)
    }

    public func isStatusActive(_ flag: NativeApplicationStatusFlags) -> Bool {
        return (self.status.Status & flag.rawValue) != 0
    }

    public func getStatus() -> NativeApplicationStatus {
        return self.status
    }
    
    public func setStatus(_ flag: NativeApplicationStatusFlags, _ value: Int) {
        self.status.Status = status.Status | UInt32(value) << Int(flag.rawValue - 1)
    }

    public static func release(_ pointer: UnsafeRawPointer) {
        Unmanaged<MacOSApplication>.fromOpaque(pointer).release()
    }

    public static func fromPointer(_ pointer: UnsafeRawPointer) -> MacOSApplication {
        return Unmanaged<MacOSApplication>.fromOpaque(pointer).takeUnretainedValue()
    }
}


class MacOSAppDelegate: NSObject, NSApplicationDelegate {
    private let application: MacOSApplication

    public init(_ application: MacOSApplication) {
        self.application = application
    }
    
    func applicationShouldTerminate(_ sender: NSApplication) -> NSApplication.TerminateReply {
        self.application.setStatus(Closing, 1)
        return NSApplication.TerminateReply.terminateCancel
    }
}