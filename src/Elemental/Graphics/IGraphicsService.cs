namespace Elemental.Graphics;

/// <summary>
/// Manages low level GPU graphics resources and commands.
/// </summary>
[PlatformService(InitMethod = nameof(PlatformServiceInterop.Native_GraphicsServiceInit), DisposeMethod = nameof(PlatformServiceInterop.Native_GraphicsServiceDispose))] 
public interface IGraphicsService
{
    /// <summary>
    /// Gets the list of available graphics devices.
    /// </summary>
    /// <returns>A list of <see cref="GraphicsDeviceInfo" /> objects.</returns>
    //ReadOnlySpan<GraphicsDeviceInfo> GetAvailableGraphicsDevices();

    /// <summary>
    /// Creates a new <see cref="GraphicsDevice" />.
    /// </summary>
    /// <param name="options">Optional parameters used by the graphics device creation.</param>
    /// <returns>Graphics device handle.</returns>
    GraphicsDevice CreateGraphicsDevice(GraphicsDeviceOptions options);

    /// <summary>
    /// Deletes the specified <see cref="GraphicsDevice" />.
    /// </summary>
    /// <param name="graphicsDevice">Graphics device to delete.</param>
    void DeleteGraphicsDevice(GraphicsDevice graphicsDevice);

    /// <summary>
    /// Gets informations about the specified <see cref="GraphicsDevice" />.
    /// </summary>
    /// <param name="graphicsDevice">Graphics device handle.</param>
    /// <returns>Informations about the graphics device.</returns>     
    GraphicsDeviceInfo GetGraphicsDeviceInfo(GraphicsDevice graphicsDevice);
}