namespace Elemental;

internal static partial class PlatformServiceInterop
{
    [LibraryImport("Elemental.Native")]
    internal static partial nint Native_GraphicsServiceInit();
    
    [LibraryImport("Elemental.Native")]
    internal static partial nint Native_GraphicsServiceDispose();
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_GetAvailableGraphicsDevices(out int graphicsDeviceCount, [MarshalUsing(CountElementName = nameof(graphicsDeviceCount))]out Span<GraphicsDeviceInfo> graphicsDevices);

    [LibraryImport("Elemental.Native")]
    internal static partial nint Native_CreateGraphicsDevice(GraphicsDeviceOptions options);

    [LibraryImport("Elemental.Native")]
    internal static partial void Native_DeleteGraphicsDevice(nint graphicsDevice);
    
    [LibraryImport("Elemental.Native")]
    internal static partial GraphicsDeviceInfo Native_GetGraphicsDeviceInfo(nint graphicsDevice);
}