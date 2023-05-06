namespace Elemental.Inputs;

/// <summary>
/// Manages input devices.
/// </summary>
[PlatformService(InitMethod = nameof(PlatformServiceInterop.Native_InitInputsService), DisposeMethod = nameof(PlatformServiceInterop.Native_FreeInputsService))]
public interface IInputsService
{
    // TODO: Allows the user to configure the gamepad deadzone (shape: rectangular, circle, etc and value)
    InputState GetInputState(NativeApplication application);

    /*

    var keyboard = inputService.GetKeyboard();

    if (inputService.IsPressed(keyboard.KeyA))
    {
        ...
    }

    var gamepad = inputService.GetGamepad(0);

    if (inputService.IsPressed(gamepad.Button1))
    {

    }

    var xMovement = inputService.GetValue(gamepad.LeftStickX);

    struct InputObjectState
    {
        public float Value;
        public float PreviousValue;
        public byte Status; (IsPressed, IsReleased, HasRepeatChanged)
        public byte RepeatCount;
        public byte PreviousRepeatCount;
    }
    
    inputService.GetInputObjectState(InputObject.KeyA);
    inputService.GetInputObjectState(InputObject.Gamepad1Button1);


    inputService.GetInputObjectStates(gamepad);
    */

}