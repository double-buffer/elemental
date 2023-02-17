namespace Elemental.Graphics;

/// <summary>
/// Represents a texture resource that can be used by the GPU.
/// </summary>
[PlatformNativePointer(/*DeleteMethod = nameof(PlatformServiceInterop.Native_FreeCommandList)*/)]
public readonly partial record struct Texture
{
}