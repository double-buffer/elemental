namespace Elemental;

internal unsafe static partial class PlatformServiceInterop
{
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_InitInputsService();
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeInputsService();
    
    [LibraryImport("Elemental.Native")]
    internal static partial InputState Native_GetInputState(NativeApplication application);
}