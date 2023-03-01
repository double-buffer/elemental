import Metal

public class MetalObject<T: AnyObject, TMetal> {
    public init(_ metalDevice: MTLDevice, _ deviceObject : TMetal) {
        self.metalDevice = metalDevice
        self.deviceObject = deviceObject
    }
    
    public let metalDevice: MTLDevice
    public let deviceObject: TMetal

    public func toPointer() -> UnsafeMutableRawPointer {
        return Unmanaged.passRetained(self).toOpaque()
    }
    
    public func toPointerUnretained() -> UnsafeMutableRawPointer {
        return Unmanaged.passUnretained(self).toOpaque()
    }

    public static func release(_ pointer: UnsafeRawPointer) {
        Unmanaged<T>.fromOpaque(pointer).release()
    }

    public static func fromPointer(_ pointer: UnsafeRawPointer) -> T {
        return Unmanaged<T>.fromOpaque(pointer).takeUnretainedValue()
    }
}
