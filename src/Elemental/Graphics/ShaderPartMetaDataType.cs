namespace Elemental.Graphics;

/// <summary>
/// Enumerates the available shader part meta data.
/// </summary>
public enum ShaderPartMetaDataType
{
    /// <summary>
    /// Number of push constants the shader use.
    /// </summary>
    PushConstantsCount,
    
    /// <summary>
    /// Thread count X it the shader is a compute, amplification or mesh shader.
    /// </summary>
    ThreadCountX,
    
    /// <summary>
    /// Thread count Y it the shader is a compute, amplification or mesh shader.
    /// </summary>
    ThreadCountY,
    
    /// <summary>
    /// Thread count Z it the shader is a compute, amplification or mesh shader.
    /// </summary>
    ThreadCountZ
}