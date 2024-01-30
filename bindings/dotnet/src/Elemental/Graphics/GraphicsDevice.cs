namespace Elemental.Graphics;

/// <summary>
/// Manages all GPU device operations.
/// </summary>
[PlatformNativePointer(DeleteMethod = nameof(PlatformServiceInterop.Native_FreeGraphicsDevice))]
public readonly partial record struct GraphicsDevice
{
}