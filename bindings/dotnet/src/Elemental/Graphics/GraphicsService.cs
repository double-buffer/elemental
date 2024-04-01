namespace Elemental.Graphics;

/// <inheritdoc />
public class GraphicsService : IGraphicsService
{
    public void SetGraphicsOptions(in GraphicsOptions options = default)
    {
        GraphicsServiceInterop.SetGraphicsOptions(options);
    }

    public GraphicsDeviceInfoSpan GetAvailableGraphicsDevices()
    {
        return GraphicsServiceInterop.GetAvailableGraphicsDevices();
    }

    public GraphicsDevice CreateGraphicsDevice(in GraphicsDeviceOptions options = default)
    {
        return GraphicsServiceInterop.CreateGraphicsDevice(options);
    }

    public void FreeGraphicsDevice(GraphicsDevice graphicsDevice)
    {
        GraphicsServiceInterop.FreeGraphicsDevice(graphicsDevice);
    }

    public GraphicsDeviceInfo GetGraphicsDeviceInfo(GraphicsDevice graphicsDevice)
    {
        return GraphicsServiceInterop.GetGraphicsDeviceInfo(graphicsDevice);
    }

    public unsafe CommandQueue CreateCommandQueue(GraphicsDevice graphicsDevice, CommandQueueType type, in CommandQueueOptions options = default)
    {
        fixed (byte* DebugNamePinned = options.DebugName)
        {
            var optionsUnsafe = new CommandQueueOptionsUnsafe();
            optionsUnsafe.DebugName = DebugNamePinned;

            return GraphicsServiceInterop.CreateCommandQueue(graphicsDevice, type, optionsUnsafe);
        }
    }

    public void FreeCommandQueue(CommandQueue commandQueue)
    {
        GraphicsServiceInterop.FreeCommandQueue(commandQueue);
    }

    public unsafe CommandList GetCommandList(CommandQueue commandQueue, in CommandListOptions options = default)
    {
        fixed (byte* DebugNamePinned = options.DebugName)
        {
            var optionsUnsafe = new CommandListOptionsUnsafe();
            optionsUnsafe.DebugName = DebugNamePinned;

            return GraphicsServiceInterop.GetCommandList(commandQueue, optionsUnsafe);
        }
    }

    public void CommitCommandList(CommandList commandList)
    {
        GraphicsServiceInterop.CommitCommandList(commandList);
    }

    public Fence ExecuteCommandList(CommandQueue commandQueue, CommandList commandList, in ExecuteCommandListOptions options = default)
    {
        return GraphicsServiceInterop.ExecuteCommandList(commandQueue, commandList, options);
    }

    public Fence ExecuteCommandLists(CommandQueue commandQueue, CommandListSpan commandLists, in ExecuteCommandListOptions options = default)
    {
        return GraphicsServiceInterop.ExecuteCommandLists(commandQueue, commandLists, options);
    }

    public void WaitForFenceOnCpu(Fence fence)
    {
        GraphicsServiceInterop.WaitForFenceOnCpu(fence);
    }

    /// <summary>
    /// TODO: ResetCommandAllocation?
    /// </summary>
    public SwapChain CreateSwapChain(CommandQueue commandQueue, Window window, in SwapChainOptions options = default)
    {
        return GraphicsServiceInterop.CreateSwapChain(commandQueue, window, options);
    }

    public void FreeSwapChain(SwapChain swapChain)
    {
        GraphicsServiceInterop.FreeSwapChain(swapChain);
    }

    public SwapChainInfo GetSwapChainInfo(SwapChain swapChain)
    {
        return GraphicsServiceInterop.GetSwapChainInfo(swapChain);
    }

    public void ResizeSwapChain(SwapChain swapChain, uint width, uint height)
    {
        GraphicsServiceInterop.ResizeSwapChain(swapChain, width, height);
    }

    public Texture GetSwapChainBackBufferTexture(SwapChain swapChain)
    {
        return GraphicsServiceInterop.GetSwapChainBackBufferTexture(swapChain);
    }

    public void PresentSwapChain(SwapChain swapChain)
    {
        GraphicsServiceInterop.PresentSwapChain(swapChain);
    }

    public void WaitForSwapChainOnCpu(SwapChain swapChain)
    {
        GraphicsServiceInterop.WaitForSwapChainOnCpu(swapChain);
    }

    public ShaderLibrary CreateShaderLibrary(GraphicsDevice graphicsDevice, DataSpan shaderLibraryData)
    {
        return GraphicsServiceInterop.CreateShaderLibrary(graphicsDevice, shaderLibraryData);
    }

    public void FreeShaderLibrary(ShaderLibrary shaderLibrary)
    {
        GraphicsServiceInterop.FreeShaderLibrary(shaderLibrary);
    }

    /// <summary>
    /// TODO: Provide Async compile methods
TODO: Provide only one generic pipeline creation method?
    /// </summary>
    public unsafe PipelineState CompileGraphicsPipelineState(GraphicsDevice graphicsDevice, in GraphicsPipelineStateParameters parameters)
    {
        fixed (byte* DebugNamePinned = parameters.DebugName)
        {
            fixed (byte* MeshShaderFunctionPinned = parameters.MeshShaderFunction)
            {
                fixed (byte* PixelShaderFunctionPinned = parameters.PixelShaderFunction)
                {
                    var parametersUnsafe = new GraphicsPipelineStateParametersUnsafe();
                    parametersUnsafe.DebugName = DebugNamePinned;
                    parametersUnsafe.ShaderLibrary = parameters.ShaderLibrary;
                    parametersUnsafe.MeshShaderFunction = MeshShaderFunctionPinned;
                    parametersUnsafe.PixelShaderFunction = PixelShaderFunctionPinned;
                    parametersUnsafe.TextureFormats = parameters.TextureFormats;

                    return GraphicsServiceInterop.CompileGraphicsPipelineState(graphicsDevice, parametersUnsafe);
                }
            }
        }
    }

    public void FreePipelineState(PipelineState pipelineState)
    {
        GraphicsServiceInterop.FreePipelineState(pipelineState);
    }

    /// <summary>
    /// TODO: Get Pipeline State Info (for compiled status etc)
ElemAPI ElemPipelineState ElemCreateComputePipelineState(ElemGraphicsDevice graphicsDevice, const ElemComputePipelineStateParameters* parameters);
TODO: Enumerate pipeline infos?
    /// </summary>
    public void BindPipelineState(CommandList commandList, PipelineState pipelineState)
    {
        GraphicsServiceInterop.BindPipelineState(commandList, pipelineState);
    }

    public void PushPipelineStateConstants(CommandList commandList, uint offsetInBytes, DataSpan data)
    {
        GraphicsServiceInterop.PushPipelineStateConstants(commandList, offsetInBytes, data);
    }

    /// <summary>
    /// TODO: Cache functions
    /// </summary>
    public void BeginRenderPass(CommandList commandList, in BeginRenderPassParameters parameters)
    {
        GraphicsServiceInterop.BeginRenderPass(commandList, parameters);
    }

    public void EndRenderPass(CommandList commandList)
    {
        GraphicsServiceInterop.EndRenderPass(commandList);
    }

    public void DispatchMesh(CommandList commandList, uint threadGroupCountX, uint threadGroupCountY, uint threadGroupCountZ)
    {
        GraphicsServiceInterop.DispatchMesh(commandList, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    }

}
