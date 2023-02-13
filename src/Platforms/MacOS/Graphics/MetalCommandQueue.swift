import Metal

protocol MetalObject<T> {
    // TODO: Make a base generic class
    let metalDevice: MTLDevice
    let deviceObject: T

    public init(_ metalDevice: MTLDevice, _ deviceObject : T) {
        self.metalDevice = metalDevice
        self.deviceObject = deviceObject
    }

    public static func release(_ pointer: UnsafeRawPointer) {
        Unmanaged<T>.fromOpaque(pointer).release()
    }

    public static func fromPointer(_ pointer: UnsafeRawPointer) -> T {
        return Unmanaged<T>.fromOpaque(pointer).takeUnretainedValue()
    }
}

public class MetalCommandQueue {
    // TODO: Make a base generic class
    let metalDevice: MTLDevice
    let deviceObject: MTLCommandQueue

    public init(_ metalDevice: MTLDevice, _ metalCommandQueue : MTLCommandQueue) {
        self.metalDevice = metalDevice
        self.deviceObject = metalCommandQueue
    }

    public static func release(_ pointer: UnsafeRawPointer) {
        Unmanaged<MetalCommandQueue>.fromOpaque(pointer).release()
    }

    public static func fromPointer(_ pointer: UnsafeRawPointer) -> MetalCommandQueue {
        return Unmanaged<MetalCommandQueue>.fromOpaque(pointer).takeUnretainedValue()
    }
}