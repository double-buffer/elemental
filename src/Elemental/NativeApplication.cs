namespace Elemental;

/// <summary>
/// Handler to a native application.
/// </summary>
[PlatformNativePointer(DeleteMethod = nameof(PlatformServiceInterop.Native_DeleteApplication))]
public readonly partial record struct NativeApplication
{
}