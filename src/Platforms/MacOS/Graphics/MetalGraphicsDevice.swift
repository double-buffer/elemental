import Metal

public class MetalGraphicsDevice {
    public init(_ metalDevice: MTLDevice) {
        self.metalDevice = metalDevice
    }
    
    public let metalDevice: MTLDevice

    public static func release(_ pointer: UnsafeRawPointer) {
        Unmanaged<MetalGraphicsDevice>.fromOpaque(pointer).release()
    }

    public static func fromPointer(_ pointer: UnsafeRawPointer) -> MetalGraphicsDevice {
        return Unmanaged<MetalGraphicsDevice>.fromOpaque(pointer).takeUnretainedValue()
    }
}