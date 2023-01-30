import Cocoa

public class MacOSWindow {
    let window: NSWindow

    init(_ window: NSWindow) {
        self.window = window
    }
    
    public static func release(_ pointer: UnsafeRawPointer) {
        Unmanaged<MacOSWindow>.fromOpaque(pointer).release()
    }
    
    public static func fromPointer(_ pointer: UnsafeRawPointer) -> MacOSWindow {
        return Unmanaged<MacOSWindow>.fromOpaque(pointer).takeUnretainedValue()
    }
}