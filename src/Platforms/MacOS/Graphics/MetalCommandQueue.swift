import Metal

public class MetalCommandQueue : MetalObject<MetalCommandQueue, MTLCommandQueue> {
    public override init(_ metalDevice: MTLDevice, _ metalCommandQueue : MTLCommandQueue) {
        super.init(metalDevice, metalCommandQueue)
    }
}