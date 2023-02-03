namespace Elemental;

/// <summary>
/// Handler to a native window.
/// </summary>
[PlatformNativePointer(DeleteMethod = nameof(PlatformServiceInterop.Native_FreeWindow))]
public readonly partial record struct NativeWindow
{
}