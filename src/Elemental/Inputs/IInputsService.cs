namespace Elemental.Inputs;

/// <summary>
/// Manages input devices.
/// </summary>
[PlatformService(InitMethod = nameof(PlatformServiceInterop.Native_InitInputsService), DisposeMethod = nameof(PlatformServiceInterop.Native_FreeInputsService))]
public interface IInputsService
{
    // TODO: Allows the user to configure the gamepad deadzone (shape: rectangular, circle, etc and value)
    InputState GetInputState(NativeApplication application);
}