
namespace Elemental;

internal static partial class GraphicsServiceInterop
{
    [LibraryImport("Elemental.Native", EntryPoint = "ElemCreateGraphicsDevice")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial GraphicsDevice CreateGraphicsDevice();

    [LibraryImport("Elemental.Native", EntryPoint = "ElemFreeGraphicsDevice")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial void FreeGraphicsDevice(GraphicsDevice graphicsDevice);

}
