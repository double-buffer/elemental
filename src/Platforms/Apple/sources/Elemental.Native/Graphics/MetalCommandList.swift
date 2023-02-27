import Metal

public class MetalCommandList : MetalObject<MetalCommandList, MTLCommandBuffer> {
    public override init(_ metalDevice: MTLDevice, _ metalCommandBuffer : MTLCommandBuffer) {
        super.init(metalDevice, metalCommandBuffer)
    }
    
    // TODO: There can be only one command encoder active at the same time
    public var commandEncoder: MTLCommandEncoder?
}