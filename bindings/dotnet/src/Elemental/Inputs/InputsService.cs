namespace Elemental.Inputs;

/// <inheritdoc />
public class InputsService : IInputsService
{
    public InputDeviceInfo GetInputDeviceInfo(InputDevice inputDevice)
    {
        return InputsServiceInterop.GetInputDeviceInfo(inputDevice);
    }

    public unsafe InputStream GetInputStream()
    {
        var resultUnsafe = InputsServiceInterop.GetInputStream();

        var result = new InputStream();
        
var EventsCounter = 0;
var EventsPointer = (byte*)resultUnsafe.Events;

while (EventsPointer[EventsCounter] != 0)
{
EventsCounter++;
}

EventsCounter++;
        var EventsSpan = new ReadOnlySpan<byte>(EventsPointer, EventsCounter);
        var EventsArray = new byte[EventsCounter];
        EventsSpan.CopyTo(EventsArray);
        result.Events = EventsArray;

        return result;
    }

}
