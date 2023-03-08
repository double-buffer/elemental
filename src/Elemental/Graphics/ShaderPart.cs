namespace Elemental.Graphics;

public readonly record struct ShaderPart
{
    public ShaderStage Stage { get; init; }
    //public ReadOnlySpan<byte> Data { get; init; }
}