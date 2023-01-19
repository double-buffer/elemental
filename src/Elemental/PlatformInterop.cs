namespace Elemental;

public static partial class NativeApplicationServiceInterop
{
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint Native_CreateApplication(string applicationName);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_RunApplication(nint application, RunHandlerDelegate runHandler);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint Native_CreateWindow(nint application, NativeWindowDescription description);
}