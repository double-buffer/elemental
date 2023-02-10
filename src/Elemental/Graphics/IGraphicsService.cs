namespace Elemental.Graphics;

/// <summary>
/// Manages low level GPU graphics resources and commands.
/// </summary>
[PlatformService(InitMethod = nameof(PlatformServiceInterop.Native_InitGraphicsService), DisposeMethod = nameof(PlatformServiceInterop.Native_FreeGraphicsService))]
public interface IGraphicsService
{
    /// <summary>
    /// Gets the list of available graphics devices.
    /// </summary>
    /// <returns>A list of <see cref="GraphicsDeviceInfo" /> objects.</returns>
    ReadOnlySpan<GraphicsDeviceInfo> GetAvailableGraphicsDevices();

    /// <summary>
    /// Creates a new <see cref="GraphicsDevice" />.
    /// </summary>
    /// <param name="options">Optional parameters used by the graphics device creation.</param>
    /// <returns>Graphics device handle.</returns>
    GraphicsDevice CreateGraphicsDevice(GraphicsDeviceOptions options = default);

    /// <summary>
    /// Frees the specified <see cref="GraphicsDevice" />.
    /// </summary>
    /// <param name="graphicsDevice">Graphics device to free.</param>
    void FreeGraphicsDevice(GraphicsDevice graphicsDevice);

    /// <summary>
    /// Gets informations about the specified <see cref="GraphicsDevice" />.
    /// </summary>
    /// <param name="graphicsDevice">Graphics device handle.</param>
    /// <returns>Informations about the graphics device.</returns>     
    GraphicsDeviceInfo GetGraphicsDeviceInfo(GraphicsDevice graphicsDevice);

    /// <summary>
    /// Creates a new <see cref="CommandQueue" /> that can be used to manage command lists.
    /// </summary>
    /// <param name="graphicsDevice">Graphics device to use.</param>
    /// <param name="type">Type of the command queue.</param>
    /// <returns>Command Queue handle.</returns>
    CommandQueue CreateCommandQueue(GraphicsDevice graphicsDevice, CommandType type);

    /// <summary>
    /// Frees the specified <see cref="CommandQueue" />.
    /// </summary>
    /// <param name="commandQueue">CommandQueue to free.</param>
    void FreeCommandQueue(CommandQueue commandQueue);

    /// <summary>
    /// Sets the debug label of the specified <see cref="CommandQueue" />.
    /// </summary>
    /// <param name="commandQueue">Comand queue to use.</param>
    /// <param name="label">Debug label that will be used by trace tools.</param>
    void SetCommandQueueLabel(CommandQueue commandQueue, string label);

    /*
    CommandList CreateCommandList(CommandType type);
    void SetCommandListLabel(CommandList commandList);
    void CommitCommandList(CommandList commandList);

    Fence ExecuteCommandLists(CommandQueue commandQueue, ReadOnlyList<CommandList> commandLists, ReadOnlyList<Fence> fenceToWait);
    */
}