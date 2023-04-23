import Metal

public class MetalTexture : MetalObject<MetalTexture> {
    public init(_ graphicsDevice: MetalGraphicsDevice, _ metalTexture : MTLTexture) {
        self.deviceObject = metalTexture
        self.isPresentTexture = false
        super.init(graphicsDevice)
    }

    public let deviceObject: MTLTexture
    public var isPresentTexture: Bool
}