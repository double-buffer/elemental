namespace Elemental;

internal unsafe static partial class PlatformServiceInterop
{
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_InitGraphicsService(GraphicsServiceOptions options = default);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeGraphicsService();

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
    
    [LibraryImport("Elemental.Native")]
    internal static partial nint Native_CreateCommandQueue(nint graphicsDevice, CommandQueueType type);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeCommandQueue(nint commandQueue);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_SetCommandQueueLabel(nint commandQueue, string label);
    
    [LibraryImport("Elemental.Native")]
    internal static partial nint Native_CreateCommandList(nint commandQueue);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeCommandList(nint commandList);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_SetCommandListLabel(nint commandList, string label);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_CommitCommandList(nint commandList);
}