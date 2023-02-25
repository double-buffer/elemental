namespace Elemental.Graphics;

// TODO: It would be maybe nice to have a textureview object that behaves like a texture object?

/// <summary>
/// Represents a texture resource that can be used by the GPU.
/// </summary>
[PlatformNativePointer(DeleteMethod = nameof(PlatformServiceInterop.Native_FreeTexture))]
public readonly partial record struct Texture
{
}