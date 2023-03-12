import Metal

public class MetalObject<T: AnyObject> {
    public init(_ graphicsDevice: MetalGraphicsDevice) {
        self.graphicsDevice = graphicsDevice
    }
    
    public let graphicsDevice: MetalGraphicsDevice

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
