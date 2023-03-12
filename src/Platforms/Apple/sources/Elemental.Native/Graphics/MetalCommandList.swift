import Metal
import NativeElemental

public class MetalCommandList : MetalObject<MetalCommandList> {
    public init(_ graphicsDevice: MetalGraphicsDevice, _ metalCommandBuffer : MTLCommandBuffer) {
        self.deviceObject = metalCommandBuffer
        self.isRenderPassActive = false

        super.init(graphicsDevice)
    }

    public let deviceObject : MTLCommandBuffer
    // TODO: There can be only one command encoder active at the same time
    public var commandEncoder: MTLCommandEncoder?
    public var isRenderPassActive: Bool
    public var currentRenderPassDescriptor: RenderPassDescriptor?
}