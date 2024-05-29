namespace Elemental.Inputs;

/// <summary>
/// Defines an interface for Inputs services.
/// </summary>
public interface IInputsService
{
    InputDeviceInfo GetInputDeviceInfo(InputDevice inputDevice);

    InputStream GetInputStream();
}
