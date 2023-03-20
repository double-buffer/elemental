namespace Elemental.Graphics;

public readonly record struct ShaderPartMetaData
{
    public required ShaderPartMetaDataType Type { get; init; }
    public required uint Value { get; init; }
}