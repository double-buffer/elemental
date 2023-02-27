import Metal
import QuartzCore.CAMetalLayer

public class MetalSwapChain : MetalObject<MetalSwapChain, CAMetalLayer> {
    public init(_ metalDevice: MTLDevice, _ deviceObject : CAMetalLayer, _ commandQueue: MetalCommandQueue, _ presentSemaphore: DispatchSemaphore) {
        self.commandQueue = commandQueue
        self.presentSemaphore = presentSemaphore
        super.init(metalDevice, deviceObject)
    }

    public let commandQueue: MetalCommandQueue
    public let presentSemaphore: DispatchSemaphore
    public var backBufferDrawable: CAMetalDrawable?
}