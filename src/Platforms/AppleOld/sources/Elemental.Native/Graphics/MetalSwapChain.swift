import Metal
import QuartzCore.CAMetalLayer

public class MetalSwapChain : MetalObject<MetalSwapChain> {
    public init(_ graphicsDevice: MetalGraphicsDevice, _ deviceObject : CAMetalLayer, _ commandQueue: MetalCommandQueue, _ presentSemaphore: DispatchSemaphore) {
        self.deviceObject = deviceObject
        self.commandQueue = commandQueue
        self.presentSemaphore = presentSemaphore
        super.init(graphicsDevice)
    }

    public let deviceObject: CAMetalLayer
    public let commandQueue: MetalCommandQueue
    public let presentSemaphore: DispatchSemaphore
    public var backBufferDrawable: CAMetalDrawable?
}