namespace Elemental.Inputs;


[PlatformNativePointer(DeleteMethod = nameof(PlatformServiceInterop.Native_FreeInputsQueue))]
public readonly partial record struct InputsQueue
{
}

public enum InputsValueId
{
    GamepadLeftStickX,
    GamepadLeftStickY,
    GamepadRightStickX,
    GamepadRightStickY,
    GamepadDpadUp,
    GamepadDpadRight,
    GamepadDpadDown,
    GamepadDpadLeft,
    GamepadButton1,
    GamepadButton2,
    GamepadButton3,
    GamepadButton4,
    GamepadLeftShoulder,
    GamepadRightShoulder
}

[NativeMarshalling(typeof(InputsValueMarshaller))]
public readonly record struct InputsValue
{
    public uint DeviceId { get; init; }
    public InputsValueId Id { get; init; }
    public ulong Timestamp { get; init; }
    public float Value { get; init; }
}

[CustomMarshaller(typeof(InputsValue), MarshalMode.Default, typeof(InputsValueMarshaller))]
internal static unsafe class InputsValueMarshaller
{
    internal struct InputsValueUnmanaged
    {
        public uint DeviceId { get; }
        public InputsValueId Id { get; }
        public ulong Timestamp { get; }
        public float Value { get; }
    }

    public static InputsValueUnmanaged ConvertToUnmanaged(InputsValue _)
    {
        return new InputsValueUnmanaged();
    }

    public static InputsValue ConvertToManaged(InputsValueUnmanaged unmanaged)
    {
        return new InputsValue
        {
            DeviceId = unmanaged.DeviceId,
            Id = unmanaged.Id,
            Timestamp = unmanaged.Timestamp,
            Value = unmanaged.Value
        };
    }
    
    public static void Free(InputsValueUnmanaged _)
    {
    }
}

/// <summary>
/// Manages input devices.
/// </summary>
[PlatformService(InitMethod = nameof(PlatformServiceInterop.Native_InitInputsService), DisposeMethod = nameof(PlatformServiceInterop.Native_FreeInputsService))]
public interface IInputsService
{
    // Old Code
    InputState GetInputState(NativeApplication application);


    // TODO: Add get devices
    // TODO: Allows the user to configure the gamepad deadzone (shape: rectangular, circle, etc and value)
    // TODO: Adds device filters, max items, etc.
    InputsQueue CreateInputsQueue();

    // TODO: Add delete

    // TODO: Add start timecode, etc.
    ReadOnlySpan<InputsValue> ReadInputsQueue(InputsQueue inputsQueue);


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
