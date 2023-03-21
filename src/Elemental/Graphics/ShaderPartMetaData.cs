namespace Elemental.Graphics;

/// <summary>
/// Struct that represent a meta data value for a <see cref="ShaderPart" />.
/// </summary>
public readonly record struct ShaderPartMetaData
{
    /// <summary>
    /// Gets ot sets the of the meta data value.
    /// </summary>
    /// <value>Type of the meta data value.</value>
    public required ShaderPartMetaDataType Type { get; init; }
    
    /// <summary>
    /// Gets or sets the value of the meta data.
    /// </summary>
    /// <value>UInt32 value of the meta data.</value>
    public required uint Value { get; init; }
}