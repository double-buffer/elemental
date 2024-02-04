namespace Elemental.Graphics;

/// <summary>
/// Represents a shader that can be executed on the GPU.
/// A shader is a collection of stage GPU programs compiled to the native API format. 
/// </summary>
[PlatformNativePointer(DeleteMethod = nameof(PlatformServiceInterop.Native_FreeShader))]
public readonly partial record struct Shader
{
}