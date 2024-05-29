namespace Elemental.Graphics;

/// <summary>
/// Parameters for creating a graphics pipeline state.
/// </summary>
public ref struct GraphicsPipelineStateParameters
{
    /// <summary>
    /// Optional debug name for the pipeline state.
    /// </summary>
    public in char DebugName { get; set; }

    /// <summary>
    /// Shader library containing the shaders.
    /// </summary>
    public ShaderLibrary ShaderLibrary { get; set; }

    /// <summary>
    /// Function name of the mesh shader in the shader library.
    /// </summary>
    public in char MeshShaderFunction { get; set; }

    /// <summary>
    /// Function name of the pixel shader in the shader library.
    /// </summary>
    public in char PixelShaderFunction { get; set; }

    /// <summary>
    /// Supported texture formats for the pipeline state.
    /// </summary>
    public ReadOnlySpan<TextureFormat> TextureFormats { get; set; }
}

internal unsafe struct GraphicsPipelineStateParametersUnsafe
{
    public in char DebugName { get; set; }

    public ShaderLibrary ShaderLibrary { get; set; }

    public in char MeshShaderFunction { get; set; }

    public in char PixelShaderFunction { get; set; }

    public TextureFormat* TextureFormats { get; set; }
}

