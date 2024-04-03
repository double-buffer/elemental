namespace Elemental.Graphics;

/// <inheritdoc />
public class GraphicsService : IGraphicsService
{
    public void SetGraphicsOptions(in GraphicsOptions options = default)
    {
        GraphicsServiceInterop.SetGraphicsOptions(options);
    }

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

    public GraphicsDevice CreateGraphicsDevice(in GraphicsDeviceOptions options = default)
    {
        return GraphicsServiceInterop.CreateGraphicsDevice(options);
    }

    public void FreeGraphicsDevice(GraphicsDevice graphicsDevice)
    {
        GraphicsServiceInterop.FreeGraphicsDevice(graphicsDevice);
    }

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

    public unsafe Fence ExecuteCommandList(CommandQueue commandQueue, CommandList commandList, in ExecuteCommandListOptions options = default)
    {
        fixed (Fence* FencesToWaitPinned = options.FencesToWait)
        {
            var optionsUnsafe = new ExecuteCommandListOptionsUnsafe();
            optionsUnsafe.InsertFence = options.InsertFence;
            optionsUnsafe.FenceAwaitableOnCpu = options.FenceAwaitableOnCpu;
            optionsUnsafe.FencesToWait = FencesToWaitPinned;

            return GraphicsServiceInterop.ExecuteCommandList(commandQueue, commandList, optionsUnsafe);
        }
    }

    public unsafe Fence ExecuteCommandLists(CommandQueue commandQueue, ReadOnlyMemory<CommandList> commandLists, in ExecuteCommandListOptions options = default)
    {
        fixed (CommandList* commandListsPinned = commandLists.Span)
        {
            fixed (Fence* FencesToWaitPinned = options.FencesToWait)
            {
                var commandListsUnsafe = new SpanUnsafe<CommandList>();
                commandListsUnsafe.Items = (nuint)commandListsPinned;
                commandListsUnsafe.Length = commandLists.Length;

                var optionsUnsafe = new ExecuteCommandListOptionsUnsafe();
                optionsUnsafe.InsertFence = options.InsertFence;
                optionsUnsafe.FenceAwaitableOnCpu = options.FenceAwaitableOnCpu;
                optionsUnsafe.FencesToWait = FencesToWaitPinned;

                return GraphicsServiceInterop.ExecuteCommandLists(commandQueue, commandListsUnsafe, optionsUnsafe);
            }
        }
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

    public unsafe ShaderLibrary CreateShaderLibrary(GraphicsDevice graphicsDevice, ReadOnlyMemory<byte> shaderLibraryData)
    {
        fixed (byte* shaderLibraryDataPinned = shaderLibraryData.Span)
        {
            var shaderLibraryDataUnsafe = new SpanUnsafe<byte>();
            shaderLibraryDataUnsafe.Items = (nuint)shaderLibraryDataPinned;
            shaderLibraryDataUnsafe.Length = shaderLibraryData.Length;

            return GraphicsServiceInterop.CreateShaderLibrary(graphicsDevice, shaderLibraryDataUnsafe);
        }
    }

    public void FreeShaderLibrary(ShaderLibrary shaderLibrary)
    {
        GraphicsServiceInterop.FreeShaderLibrary(shaderLibrary);
    }

    /// <summary>
    /// TODO: Provide Async compile methods
///TODO: Provide only one generic pipeline creation method?
    /// </summary>
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

    public void FreePipelineState(PipelineState pipelineState)
    {
        GraphicsServiceInterop.FreePipelineState(pipelineState);
    }

    /// <summary>
    /// TODO: Get Pipeline State Info (for compiled status etc)
///ElemAPI ElemPipelineState ElemCreateComputePipelineState(ElemGraphicsDevice graphicsDevice, const ElemComputePipelineStateParameters* parameters);
///TODO: Enumerate pipeline infos?
    /// </summary>
    public void BindPipelineState(CommandList commandList, PipelineState pipelineState)
    {
        GraphicsServiceInterop.BindPipelineState(commandList, pipelineState);
    }

    public unsafe void PushPipelineStateConstants(CommandList commandList, uint offsetInBytes, ReadOnlyMemory<byte> data)
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
    /// TODO: Cache functions
    /// </summary>
    public unsafe void BeginRenderPass(CommandList commandList, in BeginRenderPassParameters parameters)
    {
        fixed (RenderPassRenderTarget* RenderTargetsPinned = parameters.RenderTargets)
        {
            var parametersUnsafe = new BeginRenderPassParametersUnsafe();
            parametersUnsafe.RenderTargets = RenderTargetsPinned;

            GraphicsServiceInterop.BeginRenderPass(commandList, parametersUnsafe);
        }
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
