namespace Elemental.Graphics;

/// <summary>
/// TODO: Implement additional parameters
/// </summary>
public ref struct GraphicsPipelineStateParameters
{
    public ReadOnlySpan<byte> DebugName { get; set; }

    public ShaderLibrary ShaderLibrary { get; set; }

    public ReadOnlySpan<byte> MeshShaderFunction { get; set; }

    public ReadOnlySpan<byte> PixelShaderFunction { get; set; }

    public TextureFormatSpan TextureFormats { get; set; }
}

internal unsafe struct GraphicsPipelineStateParametersUnsafe
{
    public byte* DebugName { get; set; }

    public ShaderLibrary ShaderLibrary { get; set; }

    public byte* MeshShaderFunction { get; set; }

    public byte* PixelShaderFunction { get; set; }

    public TextureFormatSpan TextureFormats { get; set; }
}
