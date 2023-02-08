import Metal

public class MetalGraphicsDevice {
    let metalDevice: MTLDevice

    public init(_ metalDevice: MTLDevice) {
        self.metalDevice = metalDevice
    }

    public static func release(_ pointer: UnsafeRawPointer) {
        Unmanaged<MetalGraphicsDevice>.fromOpaque(pointer).release()
    }

    public static func fromPointer(_ pointer: UnsafeRawPointer) -> MetalGraphicsDevice {
        return Unmanaged<MetalGraphicsDevice>.fromOpaque(pointer).takeUnretainedValue()
    }
}