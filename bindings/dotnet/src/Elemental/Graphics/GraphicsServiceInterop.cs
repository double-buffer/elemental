namespace Elemental.Graphics;

internal static partial class GraphicsServiceInterop
{
    [LibraryImport("Elemental.Native", EntryPoint = "ElemSetGraphicsOptions")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void SetGraphicsOptions(in GraphicsOptions options);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemGetAvailableGraphicsDevices")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial SpanUnsafe<GraphicsDeviceInfoUnsafe> GetAvailableGraphicsDevices();

    [LibraryImport("Elemental.Native", EntryPoint = "ElemCreateGraphicsDevice")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial GraphicsDevice CreateGraphicsDevice(in GraphicsDeviceOptions options);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemFreeGraphicsDevice")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void FreeGraphicsDevice(GraphicsDevice graphicsDevice);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemGetGraphicsDeviceInfo")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial GraphicsDeviceInfoUnsafe GetGraphicsDeviceInfo(GraphicsDevice graphicsDevice);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemCreateCommandQueue")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial CommandQueue CreateCommandQueue(GraphicsDevice graphicsDevice, CommandQueueType type, in CommandQueueOptionsUnsafe options);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemFreeCommandQueue")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void FreeCommandQueue(CommandQueue commandQueue);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemResetCommandAllocation")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void ResetCommandAllocation(GraphicsDevice graphicsDevice);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemGetCommandList")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial CommandList GetCommandList(CommandQueue commandQueue, in CommandListOptionsUnsafe options);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemCommitCommandList")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void CommitCommandList(CommandList commandList);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemExecuteCommandList")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial Fence ExecuteCommandList(CommandQueue commandQueue, CommandList commandList, in ExecuteCommandListOptionsUnsafe options);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemExecuteCommandLists")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial Fence ExecuteCommandLists(CommandQueue commandQueue, SpanUnsafe<CommandList> commandLists, in ExecuteCommandListOptionsUnsafe options);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemWaitForFenceOnCpu")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void WaitForFenceOnCpu(Fence fence);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemCreateSwapChain")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial SwapChain CreateSwapChain(CommandQueue commandQueue, Window window, SwapChainUpdateHandler updateHandler, in SwapChainOptions options);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemFreeSwapChain")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void FreeSwapChain(SwapChain swapChain);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemGetSwapChainInfo")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial SwapChainInfo GetSwapChainInfo(SwapChain swapChain);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemSetSwapChainTiming")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void SetSwapChainTiming(SwapChain swapChain, uint frameLatency, uint targetFPS);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemPresentSwapChain")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void PresentSwapChain(SwapChain swapChain);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemCreateShaderLibrary")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial ShaderLibrary CreateShaderLibrary(GraphicsDevice graphicsDevice, SpanUnsafe<byte> shaderLibraryData);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemFreeShaderLibrary")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void FreeShaderLibrary(ShaderLibrary shaderLibrary);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemCompileGraphicsPipelineState")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial PipelineState CompileGraphicsPipelineState(GraphicsDevice graphicsDevice, in GraphicsPipelineStateParametersUnsafe parameters);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemFreePipelineState")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void FreePipelineState(PipelineState pipelineState);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemBindPipelineState")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void BindPipelineState(CommandList commandList, PipelineState pipelineState);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemPushPipelineStateConstants")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void PushPipelineStateConstants(CommandList commandList, uint offsetInBytes, SpanUnsafe<byte> data);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemBeginRenderPass")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void BeginRenderPass(CommandList commandList, in BeginRenderPassParametersUnsafe parameters);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemEndRenderPass")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void EndRenderPass(CommandList commandList);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemSetViewport")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void SetViewport(CommandList commandList, in Viewport viewport);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemSetViewports")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void SetViewports(CommandList commandList, SpanUnsafe<Viewport> viewports);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemDispatchMesh")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void DispatchMesh(CommandList commandList, uint threadGroupCountX, uint threadGroupCountY, uint threadGroupCountZ);

}
