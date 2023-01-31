namespace Elemental.Graphics;

/// <summary>
/// Handler to a GraphicsDevice.
/// </summary>
[PlatformNativePointer(DeleteMethod = nameof(PlatformServiceInterop.Native_DeleteGraphicsDevice))]
public readonly partial record struct GraphicsDevice
{
}