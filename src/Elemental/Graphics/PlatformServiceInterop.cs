namespace Elemental;

internal unsafe static partial class PlatformServiceInterop
{
    // TODO: For the moment return a span with internally allocated data doesn't seem to be possible
    // https://github.com/dotnet/runtime/issues/79413
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_GetAvailableGraphicsDevices(GraphicsDeviceInfoMarshaller.GraphicsDeviceInfoUnmanaged* graphicsDevices, out int graphicsDeviceCount);

    [LibraryImport("Elemental.Native")]
    internal static partial nint Native_CreateGraphicsDevice(GraphicsDeviceOptions options);

    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeGraphicsDevice(nint graphicsDevice);
    
    [LibraryImport("Elemental.Native")]
    internal static partial GraphicsDeviceInfo Native_GetGraphicsDeviceInfo(nint graphicsDevice);
}