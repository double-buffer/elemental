namespace Elemental.Graphics;

/// <summary>
/// Defines an interface for Graphics services.
/// </summary>
public interface IGraphicsService
{
    void SetGraphicsOptions(in GraphicsOptions options = default);

    GraphicsDeviceInfoSpan GetAvailableGraphicsDevices();

    GraphicsDevice CreateGraphicsDevice(in GraphicsDeviceOptions options = default);

    void FreeGraphicsDevice(GraphicsDevice graphicsDevice);

    GraphicsDeviceInfo GetGraphicsDeviceInfo(GraphicsDevice graphicsDevice);

    CommandQueue CreateCommandQueue(GraphicsDevice graphicsDevice, CommandQueueType type, in CommandQueueOptions options = default);

    void FreeCommandQueue(CommandQueue commandQueue);

    CommandList GetCommandList(CommandQueue commandQueue, in CommandListOptions options = default);

    void CommitCommandList(CommandList commandList);

    Fence ExecuteCommandList(CommandQueue commandQueue, CommandList commandList, in ExecuteCommandListOptions options = default);

    Fence ExecuteCommandLists(CommandQueue commandQueue, CommandListSpan commandLists, in ExecuteCommandListOptions options = default);

    void WaitForFenceOnCpu(Fence fence);

    /// <summary>
    /// TODO: ResetCommandAllocation?
    /// </summary>
    SwapChain CreateSwapChain(CommandQueue commandQueue, Window window, in SwapChainOptions options = default);

    void FreeSwapChain(SwapChain swapChain);

    SwapChainInfo GetSwapChainInfo(SwapChain swapChain);

    void ResizeSwapChain(SwapChain swapChain, uint width, uint height);

    Texture GetSwapChainBackBufferTexture(SwapChain swapChain);

    void PresentSwapChain(SwapChain swapChain);

    void WaitForSwapChainOnCpu(SwapChain swapChain);

    ShaderLibrary CreateShaderLibrary(GraphicsDevice graphicsDevice, DataSpan shaderLibraryData);

    void FreeShaderLibrary(ShaderLibrary shaderLibrary);

    /// <summary>
    /// TODO: Provide Async compile methods
TODO: Provide only one generic pipeline creation method?
    /// </summary>
    PipelineState CompileGraphicsPipelineState(GraphicsDevice graphicsDevice, in GraphicsPipelineStateParameters parameters);

    void FreePipelineState(PipelineState pipelineState);

    /// <summary>
    /// TODO: Get Pipeline State Info (for compiled status etc)
ElemAPI ElemPipelineState ElemCreateComputePipelineState(ElemGraphicsDevice graphicsDevice, const ElemComputePipelineStateParameters* parameters);
TODO: Enumerate pipeline infos?
    /// </summary>
    void BindPipelineState(CommandList commandList, PipelineState pipelineState);

    void PushPipelineStateConstants(CommandList commandList, uint offsetInBytes, DataSpan data);

    /// <summary>
    /// TODO: Cache functions
    /// </summary>
    void BeginRenderPass(CommandList commandList, in BeginRenderPassParameters parameters);

    void EndRenderPass(CommandList commandList);

    void DispatchMesh(CommandList commandList, uint threadGroupCountX, uint threadGroupCountY, uint threadGroupCountZ);
}
