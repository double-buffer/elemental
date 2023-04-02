namespace Elemental.Inputs;

/// <summary>
/// Manages input devices.
/// </summary>
[PlatformService(InitMethod = nameof(PlatformServiceInterop.Native_InitInputsService), DisposeMethod = nameof(PlatformServiceInterop.Native_FreeInputsService))]
public interface IInputsService
{
    InputState GetInputState(NativeApplication application);
}