public class MetalGraphicsDevice {

    public init() {
    }

    public static func release(_ pointer: UnsafeRawPointer) {
        Unmanaged<MetalGraphicsDevice>.fromOpaque(pointer).release()
    }

    public static func fromPointer(_ pointer: UnsafeRawPointer) -> MetalGraphicsDevice {
        return Unmanaged<MetalGraphicsDevice>.fromOpaque(pointer).takeUnretainedValue()
    }
}