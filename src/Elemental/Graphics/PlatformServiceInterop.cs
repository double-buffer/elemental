namespace Elemental;

internal unsafe static partial class PlatformServiceInterop
{
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_InitGraphicsService(in GraphicsServiceOptions options = default);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeGraphicsService();

    // TODO: For the moment return a span with internally allocated data doesn't seem to be possible
    // https://github.com/dotnet/runtime/issues/79413
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_GetAvailableGraphicsDevices(GraphicsDeviceInfoMarshaller.GraphicsDeviceInfoUnmanaged* graphicsDevices, out int graphicsDeviceCount);

    [LibraryImport("Elemental.Native")]
    internal static partial GraphicsDevice Native_CreateGraphicsDevice(in GraphicsDeviceOptions options);

    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeGraphicsDevice(GraphicsDevice graphicsDevice);
    
    [LibraryImport("Elemental.Native")]
    internal static partial GraphicsDeviceInfo Native_GetGraphicsDeviceInfo(GraphicsDevice graphicsDevice);
    
    [LibraryImport("Elemental.Native")]
    internal static partial CommandQueue Native_CreateCommandQueue(GraphicsDevice graphicsDevice, CommandQueueType type);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeCommandQueue(CommandQueue commandQueue);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_SetCommandQueueLabel(CommandQueue commandQueue, string label);
    
    [LibraryImport("Elemental.Native")]
    internal static partial CommandList Native_CreateCommandList(CommandQueue commandQueue);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeCommandList(CommandList commandList);
    
    [LibraryImport("Elemental.Native", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void Native_SetCommandListLabel(CommandList commandList, string label);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_CommitCommandList(CommandList commandList);
    
    [LibraryImport("Elemental.Native")]
    internal static partial Fence Native_ExecuteCommandLists(CommandQueue commandQueue, ReadOnlySpan<CommandList> commandLists, int commandListCount, ReadOnlySpan<Fence> fencesToWait, int fenceToWaitCount);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_WaitForFenceOnCpu(Fence fence);
    
    [LibraryImport("Elemental.Native")]
    internal static partial SwapChain Native_CreateSwapChain(NativeWindow window, CommandQueue commandQueue, in SwapChainOptions options);

    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeSwapChain(SwapChain swapChain);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_ResizeSwapChain(SwapChain swapChain, int width, int height);
    
    [LibraryImport("Elemental.Native")]
    internal static partial Texture Native_GetSwapChainBackBufferTexture(SwapChain swapChain);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_PresentSwapChain(SwapChain swapChain);

    [LibraryImport("Elemental.Native")]
    internal static partial void Native_WaitForSwapChainOnCpu(SwapChain swapChain);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_BeginRenderPass(CommandList commandList, in RenderPassDescriptor renderPassDescriptor);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_EndRenderPass(CommandList commandList);
}