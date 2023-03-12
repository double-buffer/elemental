import Atomics
import Metal

public class MetalCommandQueue : MetalObject<MetalCommandQueue> {
    public init(_ graphicsDevice: MetalGraphicsDevice, _ metalCommandQueue : MTLCommandQueue, _ fence: MTLSharedEvent) {
        self.deviceObject = metalCommandQueue
        self.fence = fence
        self.fenceValue = ManagedAtomic<UInt64>(0)
        super.init(graphicsDevice)
    }
    
    public let deviceObject: MTLCommandQueue
    public let fence: MTLSharedEvent
    public let fenceValue: ManagedAtomic<UInt64>
}