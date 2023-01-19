import NativeElemental

public class MacOSApplication {
    private var status: NativeApplicationStatus

    public init() {
        status = NativeApplicationStatus()
        setStatus(Active)
    }

    public func isStatusActive(_ flag: NativeApplicationStatusFlags) -> Bool {
        return (status.Status & flag.rawValue) != 0
    }

    public func getStatus() -> NativeApplicationStatus {
        return status
    }
    
    public func setStatus(_ flag: NativeApplicationStatusFlags) {
        status.Status = status.Status | 1 << Int(flag.rawValue - 1)
    }

    public static func fromPointer(_ pointer: UnsafeRawPointer) -> MacOSApplication {
        return Unmanaged<MacOSApplication>.fromOpaque(pointer).takeUnretainedValue()
    }
}
