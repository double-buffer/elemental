namespace Elemental.Inputs;

// TODO: Add device properties (battery, isconnected)
[NativeMarshalling(typeof(InputStateMarshaller))]
public readonly ref struct InputState
{
    public Span<uint> InputStateData { get; init; }
    public Span<float> InputStateDataFloat { get; init; }
    public Span<InputObject> InputObjects { get; init; }

    // HACK: Don't create a new objet each time !
    public Gamepad Gamepad => new(this);

    //public KeyboardInputState Keyboard => new(InputObjects);
    //public MouseInputState Mouse => new(InputObjects);
}

[CustomMarshaller(typeof(InputState), MarshalMode.Default, typeof(InputStateMarshaller))]
internal static unsafe class InputStateMarshaller
{
    internal readonly struct InputStateUnmanaged
    {
        public nint DataPointer { get; }
        public nuint DataSize { get; }
        public nint InputObjectsPointer { get; }
        public nuint InputObjectsSize { get; }
    }

    public static InputStateUnmanaged ConvertToUnmanaged(InputState _)
    {
        return new InputStateUnmanaged();
    }

    public static InputState ConvertToManaged(InputStateUnmanaged unmanaged)
    {
        return new InputState
        {
            InputStateData = new Span<uint>(unmanaged.DataPointer.ToPointer(), (int)unmanaged.DataSize),
            InputStateDataFloat = new Span<float>(unmanaged.DataPointer.ToPointer(), (int)unmanaged.DataSize),
            InputObjects = new Span<InputObject>(unmanaged.InputObjectsPointer.ToPointer(), (int)unmanaged.InputObjectsSize)
        };
    }
    
    public static void Free(InputStateUnmanaged _)
    {
    }
}

public enum InputObjectKey
{
    Gamepad1LeftStickX,
    Gamepad1LeftStickY,
    Gamepad1RightStickX,
    Gamepad1RightStickY,
    Gamepad1DpadUp,
    Gamepad1DpadRight,
    Gamepad1DpadDown,
    Gamepad1DpadLeft,
    Gamepad1Button1,
    Gamepad1Button2,
    Gamepad1Button3,
    Gamepad1Button4,
    Gamepad1LeftShoulder,
    Gamepad1RightShoulder
}

public enum InputObjectType
{
    Digital,
    Analog
}

public readonly record struct InputObjectValueAddress
{
    public required int Offset { get; init; }
    public required int BitPosition { get; init; }
}

public readonly record struct InputObject
{
    public required InputObjectType Type { get; init; }
    public InputObjectValueAddress Value { get; init; }
    internal InputObjectValueAddress PreviousValue { get; init; }

}

public readonly ref struct InputObjectValue
{
    private readonly InputState _inputState;
    private readonly InputObjectKey _inputObjectKey;

    public InputObjectValue(InputState inputState, InputObjectKey inputObjectKey)
    {
        _inputState = inputState;
        _inputObjectKey = inputObjectKey;
    }

    public float Value
    {
        get
        {
            var inputObject = _inputState.InputObjects[(int)_inputObjectKey];

            if (inputObject.Type == InputObjectType.Analog)
            {
                return _inputState.InputStateDataFloat[inputObject.Value.Offset];
            }
            else
            {
                var rawValue = _inputState.InputStateData[inputObject.Value.Offset];
                var mask = 1 << inputObject.Value.BitPosition;
                return (rawValue & mask) != 0 ? 1.0f : 0.0f;
            }
        }
    }
    
    //public bool IsPressed => Value == 1.0f;
    //public bool IsReleased => Value == 0.0f && Value != PreviousValue;
    //public bool HasRepeatChanged => Repeatcount != PreviousRepeatCount && Repeatcount != 0;
    //public float Delta => Value - PreviousValue;
    //_inputState.InputObjects[(int)InputObjectKey.Gamepad1LeftStickX]
}

public readonly ref struct Gamepad
{
    private readonly InputState _inputState;

    public Gamepad(InputState inputState)
    {
        _inputState = inputState;
    }

    public InputObjectValue LeftStickX => new InputObjectValue(_inputState, InputObjectKey.Gamepad1LeftStickX);
    public InputObjectValue LeftStickY => new InputObjectValue(_inputState, InputObjectKey.Gamepad1LeftStickY);
    public InputObjectValue RightStickX => new InputObjectValue(_inputState, InputObjectKey.Gamepad1RightStickX);
    public InputObjectValue RightStickY => new InputObjectValue(_inputState, InputObjectKey.Gamepad1RightStickY);

    public InputObjectValue DpadUp => new InputObjectValue(_inputState, InputObjectKey.Gamepad1DpadUp);
    public InputObjectValue DpadRight => new InputObjectValue(_inputState, InputObjectKey.Gamepad1DpadRight);
    public InputObjectValue DpadDown => new InputObjectValue(_inputState, InputObjectKey.Gamepad1DpadDown);
    public InputObjectValue DpadLeft => new InputObjectValue(_inputState, InputObjectKey.Gamepad1DpadLeft);

    // TODO: Chagne button name
    public InputObjectValue Button1 => new InputObjectValue(_inputState, InputObjectKey.Gamepad1Button1);
    public InputObjectValue Button2 => new InputObjectValue(_inputState, InputObjectKey.Gamepad1Button2);
    public InputObjectValue Button3 => new InputObjectValue(_inputState, InputObjectKey.Gamepad1Button3);
    public InputObjectValue Button4 => new InputObjectValue(_inputState, InputObjectKey.Gamepad1Button4);

    public InputObjectValue LeftShoulder => new InputObjectValue(_inputState, InputObjectKey.Gamepad1LeftShoulder);
    public InputObjectValue RightShoulder => new InputObjectValue(_inputState, InputObjectKey.Gamepad1RightShoulder);
}
