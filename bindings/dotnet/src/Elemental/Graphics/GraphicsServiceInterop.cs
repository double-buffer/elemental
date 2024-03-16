namespace Elemental.Graphics;

internal static partial class GraphicsServiceInterop
{
    [LibraryImport("Elemental.Native", EntryPoint = "ElemSetGraphicsOptions")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void SetGraphicsOptions(in GraphicsOptions options);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemGetAvailableGraphicsDevices")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial GraphicsDeviceInfoList GetAvailableGraphicsDevices();

    [LibraryImport("Elemental.Native", EntryPoint = "ElemCreateGraphicsDevice")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial GraphicsDevice CreateGraphicsDevice(in GraphicsDeviceOptions options);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemFreeGraphicsDevice")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void FreeGraphicsDevice(GraphicsDevice graphicsDevice);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemGetGraphicsDeviceInfo")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial GraphicsDeviceInfo GetGraphicsDeviceInfo(GraphicsDevice graphicsDevice);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemCreateGraphicsCommandQueue")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial GraphicsCommandQueue CreateGraphicsCommandQueue(GraphicsDevice graphicsDevice, GraphicsCommandQueueType type, in GraphicsCommandQueueOptionsUnsafe options);

}
