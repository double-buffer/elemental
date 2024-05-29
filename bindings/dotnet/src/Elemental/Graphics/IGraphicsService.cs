namespace Elemental.Graphics;

/// <summary>
/// Defines an interface for Graphics services.
/// </summary>
public interface IGraphicsService
{
    /// <summary>
    /// Sets graphics options like enabling a debug layer and choosing preferred graphics API.
    /// </summary>
    /// <param name="options">Configuration options for initializing the graphics system.</param>
    void SetGraphicsOptions(in GraphicsOptions options = default);

    /// <summary>
    /// Retrieves a list of available graphics devices on the system.
    /// </summary>
    /// <returns>A span of graphics device information, encapsulating details about each available device.</returns>
    ReadOnlySpan<GraphicsDeviceInfo> GetAvailableGraphicsDevices();

    /// <summary>
    /// Creates a graphics device based on specified options.
    /// </summary>
    /// <param name="options">Configuration options for the graphics device to be created.</param>
    /// <returns>A handle to the newly created graphics device.</returns>
    GraphicsDevice CreateGraphicsDevice(in GraphicsDeviceOptions options = default);

    /// <summary>
    /// Releases resources associated with a graphics device.
    /// </summary>
    /// <param name="graphicsDevice">The graphics device to free.</param>
    void FreeGraphicsDevice(GraphicsDevice graphicsDevice);

    /// <summary>
    /// Retrieves detailed information about a specific graphics device.
    /// </summary>
    /// <param name="graphicsDevice">The graphics device to query.</param>
    /// <returns>A structure containing detailed information about the device.</returns>
    GraphicsDeviceInfo GetGraphicsDeviceInfo(GraphicsDevice graphicsDevice);

    /// <summary>
    /// Creates a command queue of a specified type on a graphics device.
    /// </summary>
    /// <param name="graphicsDevice">The device on which to create the command queue.</param>
    /// <param name="type">The type of the command queue, such as graphics or compute.</param>
    /// <param name="options">Additional options for creating the command queue.</param>
    /// <returns>A handle to the newly created command queue.</returns>
    CommandQueue CreateCommandQueue(GraphicsDevice graphicsDevice, CommandQueueType type, in CommandQueueOptions options = default);

    /// <summary>
    /// Releases resources associated with a command queue.
    /// </summary>
    /// <param name="commandQueue">The command queue to free.</param>
    void FreeCommandQueue(CommandQueue commandQueue);

    /// <summary>
    /// Resets all command allocations on a graphics device, typically used to reset the state between frames.
    /// </summary>
    /// <param name="graphicsDevice">The device whose allocations are to be reset.</param>
    void ResetCommandAllocation(GraphicsDevice graphicsDevice);

    /// <summary>
    /// Retrieves a command list from a command queue, configured according to provided options.
    /// </summary>
    /// <param name="commandQueue">The command queue from which to retrieve the command list.</param>
    /// <param name="options">Additional options for creating the command list.</param>
    /// <returns>A handle to the retrieved command list.</returns>
    CommandList GetCommandList(CommandQueue commandQueue, in CommandListOptions options = default);

    /// <summary>
    /// Commits a command list for execution, finalizing its commands and preparing them to be executed on the GPU.
    /// </summary>
    /// <param name="commandList">The command list to commit.</param>
    void CommitCommandList(CommandList commandList);

    /// <summary>
    /// Executes a single command list on a command queue, optionally waiting for specified fences before execution.
    /// </summary>
    /// <param name="commandQueue">The command queue on which to execute the command list.</param>
    /// <param name="commandList">The command list to execute.</param>
    /// <param name="options">Additional options controlling execution, such as fence dependencies.</param>
    /// <returns>A fence indicating the completion state of the command list's execution.</returns>
    Fence ExecuteCommandList(CommandQueue commandQueue, CommandList commandList, in ExecuteCommandListOptions options = default);

    /// <summary>
    /// Executes multiple command lists on a command queue, optionally waiting for specified fences before execution.
    /// </summary>
    /// <param name="commandQueue">The command queue on which to execute the command lists.</param>
    /// <param name="commandLists">A span of command lists to be executed.</param>
    /// <param name="options">Additional options controlling execution, such as fence dependencies.</param>
    /// <returns>A fence indicating the completion state of the command lists' execution.</returns>
    Fence ExecuteCommandLists(CommandQueue commandQueue, ReadOnlySpan<CommandList> commandLists, in ExecuteCommandListOptions options = default);

    /// <summary>
    /// Waits for a fence to reach its signaled state on the CPU, effectively synchronizing CPU and GPU operations.
    /// </summary>
    /// <param name="fence">The fence to wait on.</param>
    void WaitForFenceOnCpu(Fence fence);

    /// <summary>
    /// Creates a swap chain for a window, allowing rendered frames to be presented to the screen.
    /// </summary>
    /// <param name="commandQueue">The command queue associated with rendering commands for the swap chain.</param>
    /// <param name="window">The window for which the swap chain is to be created.</param>
    /// <param name="updateHandler">A callback function that is called when the swap chain is updated.</param>
    /// <param name="options">Additional options for configuring the swap chain.</param>
    /// <returns>A handle to the newly created swap chain.</returns>
    SwapChain CreateSwapChain(CommandQueue commandQueue, Window window, SwapChainUpdateHandler updateHandler, in SwapChainOptions options = default);

    /// <summary>
    /// Releases resources associated with a swap chain.
    /// </summary>
    /// <param name="swapChain">The swap chain to free.</param>
    void FreeSwapChain(SwapChain swapChain);

    /// <summary>
    /// Retrieves current information about a swap chain, such as its dimensions and format.
    /// </summary>
    /// <param name="swapChain">The swap chain to query.</param>
    /// <returns>A structure containing detailed information about the swap chain.</returns>
    SwapChainInfo GetSwapChainInfo(SwapChain swapChain);

    /// <summary>
    /// Sets timing parameters for a swap chain, controlling its frame latency and target frame rate.
    /// </summary>
    /// <param name="swapChain">The swap chain to configure.</param>
    /// <param name="frameLatency">The maximum number of frames that can be queued for display.</param>
    /// <param name="targetFPS">The target frames per second to aim for during rendering.</param>
    void SetSwapChainTiming(SwapChain swapChain, uint frameLatency, uint targetFPS);

    /// <summary>
    /// Presents the next frame in the swap chain, updating the display with new content.
    /// </summary>
    /// <param name="swapChain">The swap chain from which to present the frame.</param>
    void PresentSwapChain(SwapChain swapChain);

    /// <summary>
    /// Creates a shader library from provided binary data, allowing shaders to be loaded and used by graphics pipeline states.
    /// </summary>
    /// <param name="graphicsDevice">The device on which to create the shader library.</param>
    /// <param name="shaderLibraryData">The binary data containing the shaders.</param>
    /// <returns>A handle to the newly created shader library.</returns>
    ShaderLibrary CreateShaderLibrary(GraphicsDevice graphicsDevice, ReadOnlySpan<byte> shaderLibraryData);

    /// <summary>
    /// Releases resources associated with a shader library.
    /// </summary>
    /// <param name="shaderLibrary">The shader library to free.</param>
    void FreeShaderLibrary(ShaderLibrary shaderLibrary);

    /// <summary>
    /// Compiles a graphics pipeline state using specified shaders and configuration.
    /// </summary>
    /// <param name="graphicsDevice">The device on which to compile the pipeline state.</param>
    /// <param name="parameters">Parameters defining the pipeline state configuration.</param>
    /// <returns>A handle to the newly compiled pipeline state.</returns>
    PipelineState CompileGraphicsPipelineState(GraphicsDevice graphicsDevice, in GraphicsPipelineStateParameters parameters);

    /// <summary>
    /// Releases resources associated with a pipeline state.
    /// </summary>
    /// <param name="pipelineState">The pipeline state to free.</param>
    void FreePipelineState(PipelineState pipelineState);

    /// <summary>
    /// Binds a compiled pipeline state to a command list, preparing it for rendering operations.
    /// </summary>
    /// <param name="commandList">The command list to which the pipeline state is to be bound.</param>
    /// <param name="pipelineState">The pipeline state to bind.</param>
    void BindPipelineState(CommandList commandList, PipelineState pipelineState);

    /// <summary>
    /// Pushes constants to a bound pipeline state, allowing for quick updates to shader constants without re-binding or modifying buffers.
    /// </summary>
    /// <param name="commandList">The command list through which the constants are pushed.</param>
    /// <param name="offsetInBytes">The offset within the constant buffer at which to begin updating constants.</param>
    /// <param name="data">The data to be pushed as constants.</param>
    void PushPipelineStateConstants(CommandList commandList, uint offsetInBytes, ReadOnlySpan<byte> data);

    /// <summary>
    /// Begins a render pass, setting up the rendering targets and viewports for drawing operations.
    /// </summary>
    /// <param name="commandList">The command list on which the render pass is to be started.</param>
    /// <param name="parameters">The parameters defining the render pass configuration.</param>
    void BeginRenderPass(CommandList commandList, in BeginRenderPassParameters parameters);

    /// <summary>
    /// Ends the current render pass on a command list, finalizing drawing operations and preparing for presentation or further rendering.
    /// </summary>
    /// <param name="commandList">The command list on which the render pass is to be ended.</param>
    void EndRenderPass(CommandList commandList);

    /// <summary>
    /// Sets a single viewport for rendering on a command list.
    /// </summary>
    /// <param name="commandList">The command list on which the viewport is to be set.</param>
    /// <param name="viewport">The viewport configuration to be applied.</param>
    void SetViewport(CommandList commandList, in Viewport viewport);

    /// <summary>
    /// Sets multiple viewports for rendering on a command list.
    /// </summary>
    /// <param name="commandList">The command list on which the viewports are to be set.</param>
    /// <param name="viewports">A span of viewports to be applied.</param>
    void SetViewports(CommandList commandList, ReadOnlySpan<Viewport> viewports);

    /// <summary>
    /// Dispatches a mesh shader operation on a command list, specifying the number of thread groups in each dimension.
    /// </summary>
    /// <param name="commandList">The command list on which the mesh operation is to be dispatched.</param>
    /// <param name="threadGroupCountX">The number of thread groups in the X dimension.</param>
    /// <param name="threadGroupCountY">The number of thread groups in the Y dimension.</param>
    /// <param name="threadGroupCountZ">The number of thread groups in the Z dimension.</param>
    void DispatchMesh(CommandList commandList, uint threadGroupCountX, uint threadGroupCountY, uint threadGroupCountZ);
}
