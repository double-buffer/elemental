namespace Elemental;

internal unsafe static partial class PlatformServiceInterop
{
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_InitInputsService();
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeInputsService();
    
    [LibraryImport("Elemental.Native")]
    internal static partial InputState Native_GetInputState(NativeApplication application);
    
    [LibraryImport("Elemental.Native")]
    internal static partial InputsQueue Native_CreateInputsQueue();

    [LibraryImport("Elemental.Native")]
    internal static partial void Native_FreeInputsQueue(InputsQueue inputsQueue);
    
    [LibraryImport("Elemental.Native")]
    internal static partial void Native_ReadInputsQueue(InputsQueue inputsQueue, InputsValueMarshaller.InputsValueUnmanaged* inputsValues, out int inputsValueCount);
}
