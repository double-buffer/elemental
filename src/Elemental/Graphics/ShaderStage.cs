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
    /// Amplification shader stage.
    /// </summary>
    AmplificationShader = 1,

    /// <summary>
    /// Mesh shader stage.
    /// </summary>
    MeshShader = 2,

    /// <summary>
    /// Pixel shader stage.
    /// </summary>
    PixelShader = 3
}