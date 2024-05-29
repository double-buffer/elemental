namespace Elemental.Inputs;

internal static partial class InputsServiceInterop
{
    [LibraryImport("Elemental.Native", EntryPoint = "ElemGetInputDeviceInfo")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial InputDeviceInfo GetInputDeviceInfo(InputDevice inputDevice);

    [LibraryImport("Elemental.Native", EntryPoint = "ElemGetInputStream")]
    [UnmanagedCallConv(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static partial InputStreamUnsafe GetInputStream();

}
