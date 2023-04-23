import Metal

public class PipelineStateCacheItem {
    public init(_ pipelineState: MTLRenderPipelineState) {
        self.pipelineState = pipelineState
    }

    public var pipelineState: MTLRenderPipelineState
}

public class MetalGraphicsDevice {
    public init(_ metalDevice: MTLDevice) {
        self.metalDevice = metalDevice
        self.pipelineStates = [:]
    }
    
    public let metalDevice: MTLDevice

    public var pipelineStates: [UInt64:PipelineStateCacheItem]

    public func toPointer() -> UnsafeMutableRawPointer {
        return Unmanaged.passRetained(self).toOpaque()
    }

    public static func release(_ pointer: UnsafeRawPointer) {
        Unmanaged<MetalGraphicsDevice>.fromOpaque(pointer).release()
    }

    public static func fromPointer(_ pointer: UnsafeRawPointer) -> MetalGraphicsDevice {
        return Unmanaged<MetalGraphicsDevice>.fromOpaque(pointer).takeUnretainedValue()
    }
}