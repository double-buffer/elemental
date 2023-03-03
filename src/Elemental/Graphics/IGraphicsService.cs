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
    GraphicsDevice CreateGraphicsDevice(in GraphicsDeviceOptions options = default);

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
    CommandQueue CreateCommandQueue(GraphicsDevice graphicsDevice, CommandQueueType type);

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
 
    /// <summary>
    /// Creates a new <see cref="CommandList" />.
    /// </summary>
    /// <param name="commandQueue"><see cref="CommandQueue" /> used by the command list.</param>
    /// <returns>Command list handle.</returns>
    CommandList CreateCommandList(CommandQueue commandQueue);
    
    /// <summary>
    /// Frees the specified <see cref="CommandList" />.
    /// </summary>
    /// <param name="commandList">CommandList to free.</param>
    void FreeCommandList(CommandList commandList);
    
    /// <summary>
    /// Sets the debug label of the specified <see cref="CommandList" />.
    /// </summary>
    /// <param name="commandList">Comand list to use.</param>
    /// <param name="label">Debug label that will be used by trace tools.</param>
    void SetCommandListLabel(CommandList commandList, string label);

    /// <summary>
    /// Commits the the specified <see cref="CommandList" />. After commiting, the command list
    /// cannot be used to record additional gpu commands.
    /// </summary>
    /// <param name="commandList">Command list to commit.</param>
    void CommitCommandList(CommandList commandList);

    /// <summary>
    /// Executes a <see cref="CommandList" /> on the specified <see cref="CommandQueue" />.
    /// </summary>
    /// <param name="commandQueue">CommandQueue to use.</param>
    /// <param name="commandList">Command list to execute.</param>
    /// <returns>Fence object that will be signaled when the execution is finished on the GPU.</returns>
    [PlatformMethodIgnore]
    Fence ExecuteCommandList(CommandQueue commandQueue, CommandList commandList);

    /// <summary>
    /// Executes a list of <see cref="CommandList" /> on the specified <see cref="CommandQueue" />.
    /// </summary>
    /// <param name="commandQueue">Command queue to use.</param>
    /// <param name="commandLists">List of command lists.</param>
    /// <param name="fencesToWait">List of fence objects to wait before executing the commandlists on the GPU.</param>
    /// <returns>Fence object that will be signaled when the execution is finished on the GPU.</returns>
    Fence ExecuteCommandLists(CommandQueue commandQueue, ReadOnlySpan<CommandList> commandLists, ReadOnlySpan<Fence> fencesToWait);

    /// <summary>
    /// Blocks the CPU thread until the GPU has reach the specific <see cref="Fence" />.
    /// </summary>
    /// <param name="fence">Fence to wait.</param>
    void WaitForFenceOnCpu(Fence fence);

    /// <summary>
    /// Resets the command allocation to reuse memory. This method is called automatically by the present method.
    /// </summary> 
    /// <param name="graphicsDevice">Graphics device to free.</param>
    void ResetCommandAllocation(GraphicsDevice graphicsDevice);

    /// <summary>
    /// Creates a new swap chain that can be presented to the screen.
    /// </summary>
    /// <param name="window"><see cref="NativeWindow" /> object that will be used to host the swap chain.</param>
    /// <param name="commandQueue"><see cref="CommandQueue" /> object that will be used to present the swap chain.</param>
    /// <param name="options">Optional parameters used by the swap chain creation.</param> 
    /// <returns>SwapChain handle.</returns> 
    [PlatformMethodOverride] 
    SwapChain CreateSwapChain(NativeWindow window, CommandQueue commandQueue, in SwapChainOptions options = default);

    /// <summary>
    /// Frees the specified <see cref="SwapChain" />.
    /// </summary>
    /// <param name="swapChain">SwapChain to free.</param>
    void FreeSwapChain(SwapChain swapChain);

    /// <summary>
    /// Resizes the specified <see cref="SwapChain" />.
    /// </summary>
    /// <param name="swapChain">SwapChain to resize.</param>
    /// <param name="width">New width value.</param>
    /// <param name="height">New height value.</param>
    void ResizeSwapChain(SwapChain swapChain, int width, int height);

    /// <summary>
    /// Gets the current back buffer <see cref="Texture" />of the specified <see cref="SwapChain" />.
    /// </summary>
    /// <param name="swapChain">SwapChain to use.</param>
    /// <returns>Backbuffer texture.</returns>
    Texture GetSwapChainBackBufferTexture(SwapChain swapChain);

    /// <summary>
    /// Presents the specified <see cref="SwapChain" /> to the screen. Notes that this function is not blocking.
    /// This method calls automatically ResetCommandAllocation(). 
    /// </summary>
    /// <param name="swapChain">Swap chain to present.</param>
    void PresentSwapChain(SwapChain swapChain);

    /// <summary>
    /// Blocks the CPU thread until a swap chain back buffer is available.
    /// </summary>
    /// <param name="swapChain">Swap chain to wait.</param>
    void WaitForSwapChainOnCpu(SwapChain swapChain);

    /// <summary>
    /// Begins a new render pass on the specified <see cref="CommandList" />.
    /// </summary>
    /// <param name="commandList">Command list.</param>
    /// <param name="renderPassDescriptor">Full description of the render pass.</param>
    // TODO: Add validations 
    void BeginRenderPass(CommandList commandList, in RenderPassDescriptor renderPassDescriptor);

    /// <summary>
    /// Ends the currently active render pass on the specified <see cref="CommandList" />.
    /// </summary>
    /// <param name="commandList">Command list.</param>
    void EndRenderPass(CommandList commandList);
}