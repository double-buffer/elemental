namespace Elemental.Inputs;

public record struct InputDeviceInfo
{
    public InputDevice Handle { get; set; }

    public InputDeviceType DeviceType { get; set; }
}
