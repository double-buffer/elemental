namespace Elemental;

/// <summary>
/// Handler to a native window.
/// </summary>
[PlatformNativePointer(DeleteMethod = nameof(PlatformServiceInterop.Native_DeleteWindow))]
public readonly partial record struct NativeWindow
{
}