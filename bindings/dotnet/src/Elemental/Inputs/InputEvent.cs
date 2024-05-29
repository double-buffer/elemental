namespace Elemental.Inputs;

public record struct InputEvent
{
    public Window Window { get; set; }

    public InputDevice InputDevice { get; set; }

    public uint InputDeviceTypeIndex { get; set; }

    public InputId InputId { get; set; }

    public InputType InputType { get; set; }

    public float Value { get; set; }

    public double ElapsedSeconds { get; set; }
}
