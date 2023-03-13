namespace Elemental.Graphics;

public sealed partial class GraphicsService
{
    /// <summary>
    /// Executes a <see cref="CommandList" /> on the specified <see cref="CommandQueue" />.
    /// </summary>
    /// <param name="commandQueue">CommandQueue to use.</param>
    /// <param name="commandList">Command list to execute.</param>
    /// <returns>Fence object that will be signaled when the execution is finished on the GPU.</returns>
    public Fence ExecuteCommandList(CommandQueue commandQueue, CommandList commandList)
    {
        var commandLists = MemoryMarshal.CreateReadOnlySpan(ref commandList, 1);
        return ExecuteCommandLists(commandQueue, commandLists, Array.Empty<Fence>());
    }
    
    /// <inheritdoc cref="IGraphicsService" />
    public SwapChain CreateSwapChain(NativeWindow window, CommandQueue commandQueue, in SwapChainOptions options = default)
    {
        var localOptions = (options == default) ? new SwapChainOptions() : options;

        if (localOptions.MaximumFrameLatency < 1 || localOptions.MaximumFrameLatency > 3)
        {
            throw new ArgumentOutOfRangeException(nameof(options), localOptions.MaximumFrameLatency, "Maximum Frame Latency should be between 1 and 3 (included).");
        }

        return CreateSwapChainImplementation(window, commandQueue, localOptions);
    }
    
    /// <inheritdoc cref="IGraphicsService" />
    public Shader CreateShader(GraphicsDevice graphicsDevice, ReadOnlySpan<ShaderPart> shaderParts)
    {
        // TODO: Add validation checks

        return CreateShaderImplementation(graphicsDevice, shaderParts);
    }
    
    /// <inheritdoc cref="IGraphicsService" />
    public void SetShaderConstants<T>(CommandList commandList, uint slot, ref T value) where T : struct
    {
        var byteData = MemoryMarshal.Cast<T, byte>(MemoryMarshal.CreateReadOnlySpan(ref value, 1));
        SetShaderConstants(commandList, slot, byteData);
    }
}