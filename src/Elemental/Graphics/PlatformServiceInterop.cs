namespace Elemental;

internal static partial class PlatformServiceInterop
{
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint Native_GraphicsServiceInit();
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint Native_GraphicsServiceDispose();
    
    //[LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    //internal static partial ReadOnlySpan<int> GetAvailableGraphicsDevices();

    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint Native_CreateGraphicsDevice(GraphicsDiagnostics graphicsDiagnostics);

    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_DeleteGraphicsDevice(nint graphicsDevice);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial GraphicsDeviceInfo Native_GetGraphicsDeviceInfo(nint graphicsDevice);
}