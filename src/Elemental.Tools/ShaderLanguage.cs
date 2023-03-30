namespace Elemental.Tools;

/// <summary>
/// Enumerates the supported shader languages of the shader compiler.
/// </summary>
public enum ShaderLanguage
{
    /// <summary>
    /// Unknown Shader Language.
    /// </summary>
    Unknown = 0,

    /// <summary>
    /// DirectX Shader Language.
    /// </summary>
    Hlsl = 1,

    /// <summary>
    /// Metal Shader Language.
    /// </summary>
    Msl = 2,

    /// <summary>
    /// DirectX shader bytecode.
    /// </summary>
    Dxil = 3,

    /// <summary>
    /// Vulkan shader bytecode.
    /// </summary>
    Spirv = 4,

    /// <summary>
    /// Metal shader bytecode.
    /// </summary>
    MetalIR = 5
}