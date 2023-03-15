import Metal

public class MetalShader : MetalObject<MetalShader> {
    public override init(_ graphicsDevice: MetalGraphicsDevice) {
        super.init(graphicsDevice)
    }

    public var amplificationShader: MTLFunction?
    public var meshShader: MTLFunction?
    public var pixelShader: MTLFunction?
}