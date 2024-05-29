namespace Elemental.Graphics;

/// <inheritdoc />
public class GraphicsService : IGraphicsService
{
    /// <summary>
    /// Sets graphics options like enabling a debug layer and choosing preferred graphics API.
    /// </summary>
    /// <param name="options">Configuration options for initializing the graphics system.</param>
    public void SetGraphicsOptions(in GraphicsOptions options = default)
    {
        GraphicsServiceInterop.SetGraphicsOptions(options);
    }

    /// <summary>
    /// Retrieves a list of available graphics devices on the system.
    /// </summary>
    /// <returns>A span of graphics device information, encapsulating details about each available device.</returns>
    public unsafe ReadOnlySpan<GraphicsDeviceInfo> GetAvailableGraphicsDevices()
    {
        var resultUnsafe = GraphicsServiceInterop.GetAvailableGraphicsDevices();

        var result = new GraphicsDeviceInfo[resultUnsafe.Length];
        for (int i = 0; i < resultUnsafe.Length; i++)
        {
        
var DeviceNameCounter = 0;
var DeviceNamePointer = (byte*)((GraphicsDeviceInfoUnsafe*)resultUnsafe.Items)[i].DeviceName;

while (DeviceNamePointer[DeviceNameCounter] != 0)
{
DeviceNameCounter++;
}

DeviceNameCounter++;
        var DeviceNameSpan = new ReadOnlySpan<byte>(DeviceNamePointer, DeviceNameCounter);
        var DeviceNameArray = new byte[DeviceNameCounter];
        DeviceNameSpan.CopyTo(DeviceNameArray);
        result[i].DeviceName = DeviceNameArray;
        result[i].GraphicsApi = ((GraphicsDeviceInfoUnsafe*)resultUnsafe.Items)[i].GraphicsApi;
        result[i].DeviceId = ((GraphicsDeviceInfoUnsafe*)resultUnsafe.Items)[i].DeviceId;
        result[i].AvailableMemory = ((GraphicsDeviceInfoUnsafe*)resultUnsafe.Items)[i].AvailableMemory;
        }

        return result;
    }

    /// <summary>
    /// Creates a graphics device based on specified options.
    /// </summary>
    /// <param name="options">Configuration options for the graphics device to be created.</param>
    /// <returns>A handle to the newly created graphics device.</returns>
    public GraphicsDevice CreateGraphicsDevice(in GraphicsDeviceOptions options = default)
    {
        return GraphicsServiceInterop.CreateGraphicsDevice(options);
    }

    /// <summary>
    /// Releases resources associated with a graphics device.
    /// </summary>
    /// <param name="graphicsDevice">The graphics device to free.</param>
    public void FreeGraphicsDevice(GraphicsDevice graphicsDevice)
    {
        GraphicsServiceInterop.FreeGraphicsDevice(graphicsDevice);
    }

    /// <summary>
    /// Retrieves detailed information about a specific graphics device.
    /// </summary>
    /// <param name="graphicsDevice">The graphics device to query.</param>
    /// <returns>A structure containing detailed information about the device.</returns>
    public unsafe GraphicsDeviceInfo GetGraphicsDeviceInfo(GraphicsDevice graphicsDevice)
    {
        var resultUnsafe = GraphicsServiceInterop.GetGraphicsDeviceInfo(graphicsDevice);

        var result = new GraphicsDeviceInfo();
        
var DeviceNameCounter = 0;
var DeviceNamePointer = (byte*)resultUnsafe.DeviceName;

while (DeviceNamePointer[DeviceNameCounter] != 0)
{
DeviceNameCounter++;
}

DeviceNameCounter++;
        var DeviceNameSpan = new ReadOnlySpan<byte>(DeviceNamePointer, DeviceNameCounter);
        var DeviceNameArray = new byte[DeviceNameCounter];
        DeviceNameSpan.CopyTo(DeviceNameArray);
        result.DeviceName = DeviceNameArray;
        result.GraphicsApi = resultUnsafe.GraphicsApi;
        result.DeviceId = resultUnsafe.DeviceId;
        result.AvailableMemory = resultUnsafe.AvailableMemory;

        return result;
    }

    /// <summary>
    /// Creates a command queue of a specified type on a graphics device.
    /// </summary>
    /// <param name="graphicsDevice">The device on which to create the command queue.</param>
    /// <param name="type">The type of the command queue, such as graphics or compute.</param>
    /// <param name="options">Additional options for creating the command queue.</param>
    /// <returns>A handle to the newly created command queue.</returns>
    public unsafe CommandQueue CreateCommandQueue(GraphicsDevice graphicsDevice, CommandQueueType type, in CommandQueueOptions options = default)
    {
        fixed (byte* DebugNamePinned = options.DebugName)
        {
            var optionsUnsafe = new CommandQueueOptionsUnsafe();
            optionsUnsafe.DebugName = DebugNamePinned;

            return GraphicsServiceInterop.CreateCommandQueue(graphicsDevice, type, optionsUnsafe);
        }
    }

    /// <summary>
    /// Releases resources associated with a command queue.
    /// </summary>
    /// <param name="commandQueue">The command queue to free.</param>
    public void FreeCommandQueue(CommandQueue commandQueue)
    {
        GraphicsServiceInterop.FreeCommandQueue(commandQueue);
    }

    /// <summary>
    /// Resets all command allocations on a graphics device, typically used to reset the state between frames.
    /// </summary>
    /// <param name="graphicsDevice">The device whose allocations are to be reset.</param>
    public void ResetCommandAllocation(GraphicsDevice graphicsDevice)
    {
        GraphicsServiceInterop.ResetCommandAllocation(graphicsDevice);
    }

    /// <summary>
    /// Retrieves a command list from a command queue, configured according to provided options.
    /// </summary>
    /// <param name="commandQueue">The command queue from which to retrieve the command list.</param>
    /// <param name="options">Additional options for creating the command list.</param>
    /// <returns>A handle to the retrieved command list.</returns>
    public unsafe CommandList GetCommandList(CommandQueue commandQueue, in CommandListOptions options = default)
    {
        fixed (byte* DebugNamePinned = options.DebugName)
        {
            var optionsUnsafe = new CommandListOptionsUnsafe();
            optionsUnsafe.DebugName = DebugNamePinned;

            return GraphicsServiceInterop.GetCommandList(commandQueue, optionsUnsafe);
        }
    }

    /// <summary>
    /// Commits a command list for execution, finalizing its commands and preparing them to be executed on the GPU.
    /// </summary>
    /// <param name="commandList">The command list to commit.</param>
    public void CommitCommandList(CommandList commandList)
    {
        GraphicsServiceInterop.CommitCommandList(commandList);
    }

    /// <summary>
    /// Executes a single command list on a command queue, optionally waiting for specified fences before execution.
    /// </summary>
    /// <param name="commandQueue">The command queue on which to execute the command list.</param>
    /// <param name="commandList">The command list to execute.</param>
    /// <param name="options">Additional options controlling execution, such as fence dependencies.</param>
    /// <returns>A fence indicating the completion state of the command list's execution.</returns>
    public unsafe Fence ExecuteCommandList(CommandQueue commandQueue, CommandList commandList, in ExecuteCommandListOptions options = default)
    {
        fixed (Fence* FencesToWaitPinned = options.FencesToWait)
        {
            var optionsUnsafe = new ExecuteCommandListOptionsUnsafe();
            optionsUnsafe.FenceAwaitableOnCpu = options.FenceAwaitableOnCpu;
            optionsUnsafe.FencesToWait = FencesToWaitPinned;

            return GraphicsServiceInterop.ExecuteCommandList(commandQueue, commandList, optionsUnsafe);
        }
    }

    /// <summary>
    /// Executes multiple command lists on a command queue, optionally waiting for specified fences before execution.
    /// </summary>
    /// <param name="commandQueue">The command queue on which to execute the command lists.</param>
    /// <param name="commandLists">A span of command lists to be executed.</param>
    /// <param name="options">Additional options controlling execution, such as fence dependencies.</param>
    /// <returns>A fence indicating the completion state of the command lists' execution.</returns>
    public unsafe Fence ExecuteCommandLists(CommandQueue commandQueue, ReadOnlySpan<CommandList> commandLists, in ExecuteCommandListOptions options = default)
    {
        fixed (CommandList* commandListsPinned = commandLists.Span)
        {
            fixed (Fence* FencesToWaitPinned = options.FencesToWait)
            {
                var commandListsUnsafe = new SpanUnsafe<CommandList>();
                commandListsUnsafe.Items = (nuint)commandListsPinned;
                commandListsUnsafe.Length = commandLists.Length;

                var optionsUnsafe = new ExecuteCommandListOptionsUnsafe();
                optionsUnsafe.FenceAwaitableOnCpu = options.FenceAwaitableOnCpu;
                optionsUnsafe.FencesToWait = FencesToWaitPinned;

                return GraphicsServiceInterop.ExecuteCommandLists(commandQueue, commandListsUnsafe, optionsUnsafe);
            }
        }
    }

    /// <summary>
    /// Waits for a fence to reach its signaled state on the CPU, effectively synchronizing CPU and GPU operations.
    /// </summary>
    /// <param name="fence">The fence to wait on.</param>
    public void WaitForFenceOnCpu(Fence fence)
    {
        GraphicsServiceInterop.WaitForFenceOnCpu(fence);
    }

    /// <summary>
    /// Creates a swap chain for a window, allowing rendered frames to be presented to the screen.
    /// </summary>
    /// <param name="commandQueue">The command queue associated with rendering commands for the swap chain.</param>
    /// <param name="window">The window for which the swap chain is to be created.</param>
    /// <param name="updateHandler">A callback function that is called when the swap chain is updated.</param>
    /// <param name="options">Additional options for configuring the swap chain.</param>
    /// <returns>A handle to the newly created swap chain.</returns>
    public SwapChain CreateSwapChain(CommandQueue commandQueue, Window window, SwapChainUpdateHandler updateHandler, in SwapChainOptions options = default)
    {
        return GraphicsServiceInterop.CreateSwapChain(commandQueue, window, updateHandler, options);
    }

    /// <summary>
    /// Releases resources associated with a swap chain.
    /// </summary>
    /// <param name="swapChain">The swap chain to free.</param>
    public void FreeSwapChain(SwapChain swapChain)
    {
        GraphicsServiceInterop.FreeSwapChain(swapChain);
    }

    /// <summary>
    /// Retrieves current information about a swap chain, such as its dimensions and format.
    /// </summary>
    /// <param name="swapChain">The swap chain to query.</param>
    /// <returns>A structure containing detailed information about the swap chain.</returns>
    public SwapChainInfo GetSwapChainInfo(SwapChain swapChain)
    {
        return GraphicsServiceInterop.GetSwapChainInfo(swapChain);
    }

    /// <summary>
    /// Sets timing parameters for a swap chain, controlling its frame latency and target frame rate.
    /// </summary>
    /// <param name="swapChain">The swap chain to configure.</param>
    /// <param name="frameLatency">The maximum number of frames that can be queued for display.</param>
    /// <param name="targetFPS">The target frames per second to aim for during rendering.</param>
    public void SetSwapChainTiming(SwapChain swapChain, uint frameLatency, uint targetFPS)
    {
        GraphicsServiceInterop.SetSwapChainTiming(swapChain, frameLatency, targetFPS);
    }

    /// <summary>
    /// Presents the next frame in the swap chain, updating the display with new content.
    /// </summary>
    /// <param name="swapChain">The swap chain from which to present the frame.</param>
    public void PresentSwapChain(SwapChain swapChain)
    {
        GraphicsServiceInterop.PresentSwapChain(swapChain);
    }

    /// <summary>
    /// Creates a shader library from provided binary data, allowing shaders to be loaded and used by graphics pipeline states.
    /// </summary>
    /// <param name="graphicsDevice">The device on which to create the shader library.</param>
    /// <param name="shaderLibraryData">The binary data containing the shaders.</param>
    /// <returns>A handle to the newly created shader library.</returns>
    public unsafe ShaderLibrary CreateShaderLibrary(GraphicsDevice graphicsDevice, ReadOnlySpan<byte> shaderLibraryData)
    {
        fixed (byte* shaderLibraryDataPinned = shaderLibraryData.Span)
        {
            var shaderLibraryDataUnsafe = new SpanUnsafe<byte>();
            shaderLibraryDataUnsafe.Items = (nuint)shaderLibraryDataPinned;
            shaderLibraryDataUnsafe.Length = shaderLibraryData.Length;

            return GraphicsServiceInterop.CreateShaderLibrary(graphicsDevice, shaderLibraryDataUnsafe);
        }
    }

    /// <summary>
    /// Releases resources associated with a shader library.
    /// </summary>
    /// <param name="shaderLibrary">The shader library to free.</param>
    public void FreeShaderLibrary(ShaderLibrary shaderLibrary)
    {
        GraphicsServiceInterop.FreeShaderLibrary(shaderLibrary);
    }

    /// <summary>
    /// Compiles a graphics pipeline state using specified shaders and configuration.
    /// </summary>
    /// <param name="graphicsDevice">The device on which to compile the pipeline state.</param>
    /// <param name="parameters">Parameters defining the pipeline state configuration.</param>
    /// <returns>A handle to the newly compiled pipeline state.</returns>
    public unsafe PipelineState CompileGraphicsPipelineState(GraphicsDevice graphicsDevice, in GraphicsPipelineStateParameters parameters)
    {
        fixed (byte* DebugNamePinned = parameters.DebugName)
        {
            fixed (byte* MeshShaderFunctionPinned = parameters.MeshShaderFunction)
            {
                fixed (byte* PixelShaderFunctionPinned = parameters.PixelShaderFunction)
                {
                    fixed (TextureFormat* TextureFormatsPinned = parameters.TextureFormats)
                    {
                        var parametersUnsafe = new GraphicsPipelineStateParametersUnsafe();
                        parametersUnsafe.DebugName = DebugNamePinned;
                        parametersUnsafe.ShaderLibrary = parameters.ShaderLibrary;
                        parametersUnsafe.MeshShaderFunction = MeshShaderFunctionPinned;
                        parametersUnsafe.PixelShaderFunction = PixelShaderFunctionPinned;
                        parametersUnsafe.TextureFormats = TextureFormatsPinned;

                        return GraphicsServiceInterop.CompileGraphicsPipelineState(graphicsDevice, parametersUnsafe);
                    }
                }
            }
        }
    }

    /// <summary>
    /// Releases resources associated with a pipeline state.
    /// </summary>
    /// <param name="pipelineState">The pipeline state to free.</param>
    public void FreePipelineState(PipelineState pipelineState)
    {
        GraphicsServiceInterop.FreePipelineState(pipelineState);
    }

    /// <summary>
    /// Binds a compiled pipeline state to a command list, preparing it for rendering operations.
    /// </summary>
    /// <param name="commandList">The command list to which the pipeline state is to be bound.</param>
    /// <param name="pipelineState">The pipeline state to bind.</param>
    public void BindPipelineState(CommandList commandList, PipelineState pipelineState)
    {
        GraphicsServiceInterop.BindPipelineState(commandList, pipelineState);
    }

    /// <summary>
    /// Pushes constants to a bound pipeline state, allowing for quick updates to shader constants without re-binding or modifying buffers.
    /// </summary>
    /// <param name="commandList">The command list through which the constants are pushed.</param>
    /// <param name="offsetInBytes">The offset within the constant buffer at which to begin updating constants.</param>
    /// <param name="data">The data to be pushed as constants.</param>
    public unsafe void PushPipelineStateConstants(CommandList commandList, uint offsetInBytes, ReadOnlySpan<byte> data)
    {
        fixed (byte* dataPinned = data.Span)
        {
            var dataUnsafe = new SpanUnsafe<byte>();
            dataUnsafe.Items = (nuint)dataPinned;
            dataUnsafe.Length = data.Length;

            GraphicsServiceInterop.PushPipelineStateConstants(commandList, offsetInBytes, dataUnsafe);
        }
    }

    /// <summary>
    /// Begins a render pass, setting up the rendering targets and viewports for drawing operations.
    /// </summary>
    /// <param name="commandList">The command list on which the render pass is to be started.</param>
    /// <param name="parameters">The parameters defining the render pass configuration.</param>
    public unsafe void BeginRenderPass(CommandList commandList, in BeginRenderPassParameters parameters)
    {
        fixed (RenderPassRenderTarget* RenderTargetsPinned = parameters.RenderTargets)
        {
            fixed (Viewport* ViewportsPinned = parameters.Viewports)
            {
                var parametersUnsafe = new BeginRenderPassParametersUnsafe();
                parametersUnsafe.RenderTargets = RenderTargetsPinned;
                parametersUnsafe.Viewports = ViewportsPinned;

                GraphicsServiceInterop.BeginRenderPass(commandList, parametersUnsafe);
            }
        }
    }

    /// <summary>
    /// Ends the current render pass on a command list, finalizing drawing operations and preparing for presentation or further rendering.
    /// </summary>
    /// <param name="commandList">The command list on which the render pass is to be ended.</param>
    public void EndRenderPass(CommandList commandList)
    {
        GraphicsServiceInterop.EndRenderPass(commandList);
    }

    /// <summary>
    /// Sets a single viewport for rendering on a command list.
    /// </summary>
    /// <param name="commandList">The command list on which the viewport is to be set.</param>
    /// <param name="viewport">The viewport configuration to be applied.</param>
    public void SetViewport(CommandList commandList, in Viewport viewport)
    {
        GraphicsServiceInterop.SetViewport(commandList, viewport);
    }

    /// <summary>
    /// Sets multiple viewports for rendering on a command list.
    /// </summary>
    /// <param name="commandList">The command list on which the viewports are to be set.</param>
    /// <param name="viewports">A span of viewports to be applied.</param>
    public unsafe void SetViewports(CommandList commandList, ReadOnlySpan<Viewport> viewports)
    {
        fixed (Viewport* viewportsPinned = viewports.Span)
        {
            var viewportsUnsafe = new SpanUnsafe<Viewport>();
            viewportsUnsafe.Items = (nuint)viewportsPinned;
            viewportsUnsafe.Length = viewports.Length;

            GraphicsServiceInterop.SetViewports(commandList, viewportsUnsafe);
        }
    }

    /// <summary>
    /// Dispatches a mesh shader operation on a command list, specifying the number of thread groups in each dimension.
    /// </summary>
    /// <param name="commandList">The command list on which the mesh operation is to be dispatched.</param>
    /// <param name="threadGroupCountX">The number of thread groups in the X dimension.</param>
    /// <param name="threadGroupCountY">The number of thread groups in the Y dimension.</param>
    /// <param name="threadGroupCountZ">The number of thread groups in the Z dimension.</param>
    public void DispatchMesh(CommandList commandList, uint threadGroupCountX, uint threadGroupCountY, uint threadGroupCountZ)
    {
        GraphicsServiceInterop.DispatchMesh(commandList, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    }

}
