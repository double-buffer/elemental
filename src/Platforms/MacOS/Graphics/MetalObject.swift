import Metal

public class MetalObject<T: AnyObject, TMetal> {
    public init(_ metalDevice: MTLDevice, _ deviceObject : TMetal) {
        self.metalDevice = metalDevice
        self.deviceObject = deviceObject
    }
    
    public let metalDevice: MTLDevice
    public let deviceObject: TMetal

    public static func release(_ pointer: UnsafeRawPointer) {
        Unmanaged<T>.fromOpaque(pointer).release()
    }

    public static func fromPointer(_ pointer: UnsafeRawPointer) -> T {
        return Unmanaged<T>.fromOpaque(pointer).takeUnretainedValue()
    }
}