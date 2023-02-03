namespace Elemental.Graphics;

/// <summary>
/// Contains information about a <see cref="GraphicsDevice" />.
/// </summary>
[NativeMarshalling(typeof(GraphicsDeviceInfoMarshaller))]
public readonly record struct GraphicsDeviceInfo
{
    /// <summary>
    /// Default constructor.
    /// </summary>
    public GraphicsDeviceInfo()
    {
        DeviceName = string.Empty;
        GraphicsApiName = string.Empty;
        DriverVersion = string.Empty;
    }

    /// <summary>
    /// Gets the name of the graphics device.
    /// </summary>
    /// <value>Name of the graphics device.</value>
    public string DeviceName { get; init; }

    /// <summary>
    /// Gets the name of the graphics api.
    /// </summary>
    /// <value>Name of the graphics api.</value>
    public string GraphicsApiName { get; init; }

    /// <summary>
    /// Gets the version of the graphics driver.
    /// </summary>
    /// <value>Version of the graphics driver.</value>
    public string DriverVersion { get; init; }
}

[CustomMarshaller(typeof(GraphicsDeviceInfo), MarshalMode.Default, typeof(GraphicsDeviceInfoMarshaller))]
internal static unsafe class GraphicsDeviceInfoMarshaller
{
    internal readonly struct GraphicsDeviceInfoUnmanaged
    {
        public byte* DeviceName { get; }
        public byte* GraphicsApiName { get; }
        public byte* DriverVersion { get; }
    }

    public static GraphicsDeviceInfoUnmanaged ConvertToUnmanaged(GraphicsDeviceInfo managed)
    {
        throw new NotImplementedException();
    }

    public static GraphicsDeviceInfo ConvertToManaged(GraphicsDeviceInfoUnmanaged unmanaged)
    {
        return new GraphicsDeviceInfo
        {
            DeviceName = Utf8StringMarshaller.ConvertToManaged(unmanaged.DeviceName) ?? string.Empty,
            GraphicsApiName = Utf8StringMarshaller.ConvertToManaged(unmanaged.GraphicsApiName) ?? string.Empty,
            DriverVersion = Utf8StringMarshaller.ConvertToManaged(unmanaged.DriverVersion) ?? string.Empty
        };
    }
    
    public static void Free(GraphicsDeviceInfoUnmanaged unmanaged)
    {
        PlatformServiceInterop.Native_FreeNativePointer((nint)unmanaged.DeviceName);
        PlatformServiceInterop.Native_FreeNativePointer((nint)unmanaged.GraphicsApiName);
        PlatformServiceInterop.Native_FreeNativePointer((nint)unmanaged.DriverVersion);
    }
}