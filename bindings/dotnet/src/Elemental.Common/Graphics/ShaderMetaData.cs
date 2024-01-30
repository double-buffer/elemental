namespace Elemental.Graphics;

/// <summary>
/// Struct that represent a meta data value for a Shader.
/// </summary>
public readonly record struct ShaderMetaData
{
    /// <summary>
    /// Gets ot sets the of the meta data value.
    /// </summary>
    /// <value>Type of the meta data value.</value>
    public required ShaderMetaDataType Type { get; init; }
    
    /// <summary>
    /// Gets or sets the value of the meta data.
    /// </summary>
    /// <value>UInt32 value of the meta data.</value>
    public required uint Value { get; init; }
}