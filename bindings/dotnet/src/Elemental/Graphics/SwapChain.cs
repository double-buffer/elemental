namespace Elemental.Graphics;

/// <summary>
/// Represents a collection of render targets that can be presented to the sceen.
/// </summary>
[PlatformNativePointer(DeleteMethod = nameof(PlatformServiceInterop.Native_FreeSwapChain))]
public readonly partial record struct SwapChain
{
}