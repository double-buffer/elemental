namespace Elemental.Graphics;

/// <summary>
/// Manages the command list submission to the GPU.
/// </summary>
[PlatformNativePointer(DeleteMethod = nameof(PlatformServiceInterop.Native_FreeCommandList))]
public readonly partial record struct CommandList
{
}