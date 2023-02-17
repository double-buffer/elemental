import Metal
import QuartzCore.CAMetalLayer

public class MetalSwapChain : MetalObject<MetalSwapChain, CAMetalLayer> {
    public override init(_ metalDevice: MTLDevice, _ deviceObject : CAMetalLayer) {
        super.init(metalDevice, deviceObject)
    }
}