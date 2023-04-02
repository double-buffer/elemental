namespace Elemental.Inputs;

[NativeMarshalling(typeof(InputStateMarshaller))]
public readonly ref struct InputState
{
    public Span<uint> InputStateData { get; init; }
    public Span<float> InputStateDataFloat { get; init; }
    public Span<InputObject> InputObjects { get; init; }

    //public KeyboardInputState Keyboard => new(InputObjects);
    //public MouseInputState Mouse => new(InputObjects);
}

[CustomMarshaller(typeof(InputState), MarshalMode.Default, typeof(InputStateMarshaller))]
internal static unsafe class InputStateMarshaller
{
    internal readonly struct InputStateUnmanaged
    {
        public nint DataPointer { get; }
        public int DataSize { get; }
        public nint InputObjectsPointer { get; }
        public int InputObjectsSize { get; }
    }

    public static InputStateUnmanaged ConvertToUnmanaged(InputState _)
    {
        return new InputStateUnmanaged();
    }

    public static InputState ConvertToManaged(InputStateUnmanaged unmanaged)
    {
        return new InputState
        {
            InputStateData = new Span<uint>(unmanaged.DataPointer.ToPointer(), unmanaged.DataSize),
            InputStateDataFloat = new Span<float>(unmanaged.DataPointer.ToPointer(), unmanaged.DataSize),
            InputObjects = new Span<InputObject>(unmanaged.InputObjectsPointer.ToPointer(), unmanaged.InputObjectsSize)
        };
    }
    
    public static void Free(InputStateUnmanaged _)
    {
    }
}

public enum InputObjectKey : byte
{
    Gamepad1LeftStickX,
    Gamepad1LeftStickY,
    Gamepad1Button1,
    Gamepad1Button2,
}

public enum InputObjectType : byte
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
    public required InputObjectValueAddress Value { get; init; }
    public required InputObjectValueAddress PreviousValue { get; init; }
}

public readonly record struct Gamepad
{
    
}