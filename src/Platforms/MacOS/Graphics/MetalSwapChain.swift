import Metal
import QuartzCore.CAMetalLayer

public class MetalSwapChain : MetalObject<MetalSwapChain, CAMetalLayer> {
    public init(_ metalDevice: MTLDevice, _ deviceObject : CAMetalLayer, _ commandQueue: MetalCommandQueue) {
        self.commandQueue = commandQueue
        super.init(metalDevice, deviceObject)
    }

    public let commandQueue: MetalCommandQueue
    public var backBufferDrawable: CAMetalDrawable?
}