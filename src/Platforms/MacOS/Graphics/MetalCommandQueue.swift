import Metal

public class MetalCommandQueue : MetalObject<MetalCommandQueue, MTLCommandQueue> {
    public init(_ metalDevice: MTLDevice, _ metalCommandQueue : MTLCommandQueue, _ fence: MTLSharedEvent) {
        self.fence = fence
        self.fenceValue = 0
        super.init(metalDevice, metalCommandQueue)
    }
    
    let fence: MTLSharedEvent
    var fenceValue: UInt64
}