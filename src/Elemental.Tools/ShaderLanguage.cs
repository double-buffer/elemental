namespace Elemental.Tools;

/// <summary>
/// Enumerates the supported shader languages of the shader compiler.
/// </summary>
public enum ShaderLanguage
{
    /// <summary>
    /// DirectX Shader Language.
    /// </summary>
    Hlsl,

    /// <summary>
    /// Metal Shader Language.
    /// </summary>
    Msl,
    
    /// <summary>
    /// DirectX shader bytecode.
    /// </summary>
    Dxil,
    
    /// <summary>
    /// Vulkan shader bytecode.
    /// </summary>
    Spirv,
    
    /// <summary>
    /// Metal shader bytecode.
    /// </summary>
    MetalIR
}