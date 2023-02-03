namespace Elemental.Graphics;

/// <summary>
/// Handler to a GraphicsDevice.
/// </summary>
[PlatformNativePointer(DeleteMethod = nameof(PlatformServiceInterop.Native_FreeGraphicsDevice))]
public readonly partial record struct GraphicsDevice
{
}