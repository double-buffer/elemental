import Metal

public class MetalTexture : MetalObject<MetalTexture, MTLTexture> {
    public override init(_ metalDevice: MTLDevice, _ metalTexture : MTLTexture) {
        self.isPresentTexture = false
        super.init(metalDevice, metalTexture)
    }

    public var isPresentTexture: Bool
}