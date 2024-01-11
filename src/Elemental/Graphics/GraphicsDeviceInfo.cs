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
        GraphicsApi = GraphicsApi.Unknown;
        DeviceId = 0;
        AvailableMemory = 0;
    }

    /// <summary>
    /// Gets the name of the <see cref="GraphicsDevice" />.
    /// </summary>
    /// <value>Name of the <see cref="GraphicsDevice" />.</value>
    public string DeviceName { get; init; }

    /// <summary>
    /// Gets the Graphics API used by the <see cref="GraphicsDevice" />.
    /// </summary>
    /// <value>Graphics API of the <see cref="GraphicsDevice" />.</value>
    public GraphicsApi GraphicsApi { get; init; }

    /// <summary>
    /// Gets the internal ID of the <see cref="GraphicsDevice" />.
    /// </summary>
    /// <value>Internal ID of the <see cref="GraphicsDevice" /></value>
    public ulong DeviceId { get; init; }

    /// <summary>
    /// Gets the available GPU memory in bytes of the <see cref="GraphicsDevice" />.
    /// </summary>
    /// <value>available GPU memory in bytes of the <see cref="GraphicsDevice" />.</value>
    public ulong AvailableMemory { get; init; }

    // TODO: Return maximum caps to task/mesh/compute/texture etc.
}

[CustomMarshaller(typeof(GraphicsDeviceInfo), MarshalMode.Default, typeof(GraphicsDeviceInfoMarshaller))]
internal static unsafe class GraphicsDeviceInfoMarshaller
{
    internal struct GraphicsDeviceInfoUnmanaged
    {
        public char* DeviceName;
        public GraphicsApi GraphicsApi { get; }
        public ulong DeviceId { get; }
        public ulong AvailableMemory { get; }
    }

    public static GraphicsDeviceInfoUnmanaged ConvertToUnmanaged(GraphicsDeviceInfo _)
    {
        return new GraphicsDeviceInfoUnmanaged();
    }

    public static GraphicsDeviceInfo ConvertToManaged(GraphicsDeviceInfoUnmanaged unmanaged)
    {
        return new GraphicsDeviceInfo
        {
            DeviceName = Utf8StringMarshaller.ConvertToManaged((byte*)unmanaged.DeviceName),
            GraphicsApi = unmanaged.GraphicsApi,
            DeviceId = unmanaged.DeviceId,
            AvailableMemory = unmanaged.AvailableMemory
        };
    }
    
    public static void Free(GraphicsDeviceInfoUnmanaged _)
    {
    }
}
