namespace Elemental.Graphics;

/// <summary>
/// Stage of a <see cref="Shader" />.
/// </summary>
public enum ShaderStage
{
    /// <summary>
    /// None shader stage.
    /// </summary>
    None = 0,

    /// <summary>
    /// Mesh shader stage.
    /// </summary>
    MeshShader = 1,

    /// <summary>
    /// Pixel shader stage.
    /// </summary>
    PixelShader = 2
}