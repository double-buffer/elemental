#include "HidDevices.h"
#include "Inputs.h"
#include "SystemFunctions.h"

enum HidGamepadVendor : uint32_t 
{
    HidGamepadVendor_Microsoft = 0x045E,
    HidGamepadVendor_Sony = 0x054C
};

enum HidGamepadProduct : uint32_t 
{
    HidGamepadProduct_XboxOneWirelessOldDriver = 0x02E0,
    HidGamepadProduct_XboxOneUsb = 0x02FF,
    HidGamepadProduct_XboxOneWireless = 0x02FD,
    HidGamepadProduct_DualShock4OldDriver = 0x5C4,
    HidGamepadProduct_DualShock4 = 0x9cc
};

typedef void (*ProcessHidGamepadDataPtr)(ElemWindow window, ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> hidReport, double elapsedSeconds);

struct HidGamepadHandler
{
    HidGamepadVendor Vendor;
    HidGamepadProduct Product;
    ProcessHidGamepadDataPtr ProcessDataHandler;
};

PackedStruct XboxOneWirelessOldDriverGamepadReport
{
    uint8_t Padding;
    uint16_t LeftStickX;
    uint16_t LeftStickY;
    uint16_t RightStickX;
    uint16_t RightStickY;
    uint16_t Triggers;
    uint16_t Buttons;
    uint8_t Dpad;
};

PackedStruct XboxOneWirelessGamepadReport
{
    uint8_t Padding;
    uint16_t LeftStickX;
    uint16_t LeftStickY;
    uint16_t RightStickX;
    uint16_t RightStickY;
    uint16_t LeftTrigger;
    uint16_t RightTrigger;
    uint8_t Dpad;
    uint32_t Buttons;
};

float NormalizeInputValue(uint32_t value, uint32_t maxValue, float deadZone)
{
    // TODO: Allows the configuration of deadzone
    float normalizedValue = ((float)value / (float)maxValue);

    if (normalizedValue < deadZone)
    {
        return 0.0f;
    }

    return normalizedValue;
}

float NormalizeInputValueSigned(uint32_t value, uint32_t maxValue, float deadZone)
{
    // TODO: Allows the configuration of deadzone
    float normalizedValue = ((float)value / (float)maxValue) * 2.0f - 1.0f;

    if (normalizedValue < deadZone && normalizedValue > -deadZone)
    {
        return 0.0f;
    }

    return normalizedValue;
}

void ProcessXboxOneWirelessGamepadReport(ElemWindow window, ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> hidReport, double elapsedSeconds)
{
    auto xboxReport = (XboxOneWirelessGamepadReport*)hidReport.Pointer;

    // TODO: Also here we need to send the event only if the analog value has changed
    float leftStickX = NormalizeInputValueSigned(xboxReport->LeftStickX, 65535, 0.2f);

    if (leftStickX <= 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_GamepadLeftStickXNegative,
            .InputType = ElemInputType_Analog,
            .Value = -leftStickX,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (leftStickX >= 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_GamepadLeftStickXPositive,
            .InputType = ElemInputType_Analog,
            .Value = leftStickX,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    float leftStickY = -NormalizeInputValueSigned(xboxReport->LeftStickY, 65535, 0.2f);
    
    if (leftStickY <= 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_GamepadLeftStickYNegative,
            .InputType = ElemInputType_Analog,
            .Value = -leftStickY,
            .ElapsedSeconds = elapsedSeconds
        });
    }
    // TODO: check if the value is inversed!!
    if (leftStickY >= 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_GamepadLeftStickYPositive,
            .InputType = ElemInputType_Analog,
            .Value = leftStickY,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputID_GamepadLeftTrigger,
        .InputType = ElemInputType_Analog,
        .Value = NormalizeInputValue(xboxReport->LeftTrigger, 1024, 0.1f),
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputID_GamepadRightTrigger,
        .InputType = ElemInputType_Analog,
        .Value = NormalizeInputValue(xboxReport->RightTrigger, 1024, 0.1f),
        .ElapsedSeconds = elapsedSeconds
    });

    // TODO: For buttons we need to keep track of old report to see if there was a change
    // Maybe we can do the same as with delta inputs!

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputID_GamepadButtonA,
        .InputType = ElemInputType_Digital,
        .Value = (xboxReport->Buttons & 0x01) ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputID_GamepadLeftShoulder,
        .InputType = ElemInputType_Digital,
        .Value = (xboxReport->Buttons & 0x40) ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputID_GamepadRightShoulder,
        .InputType = ElemInputType_Digital,
        .Value = (xboxReport->Buttons & 0x80) ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });
}

void ProcessXboxOneWirelessOldDriverGamepadReport(ElemWindow window, ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> hidReport, double elapsedSeconds)
{
    auto xboxReport = (XboxOneWirelessOldDriverGamepadReport*)hidReport.Pointer;

    // TODO: Also here we need to send the event only if the analog value has changed
    float leftStickX = NormalizeInputValueSigned(xboxReport->LeftStickX, 65535, 0.2f);

    if (leftStickX <= 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_GamepadLeftStickXNegative,
            .InputType = ElemInputType_Analog,
            .Value = -leftStickX,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (leftStickX >= 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_GamepadLeftStickXPositive,
            .InputType = ElemInputType_Analog,
            .Value = leftStickX,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    float leftStickY = -NormalizeInputValueSigned(xboxReport->LeftStickY, 65535, 0.2f);
    
    if (leftStickY <= 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_GamepadLeftStickYNegative,
            .InputType = ElemInputType_Analog,
            .Value = -leftStickY,
            .ElapsedSeconds = elapsedSeconds
        });
    }
    // TODO: check if the value is inversed!!
    if (leftStickY >= 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_GamepadLeftStickYPositive,
            .InputType = ElemInputType_Analog,
            .Value = leftStickY,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    const uint16_t midpoint = 32768;

    uint8_t left, right;

    // Calculate the right trigger value as a difference from midpoint
    if (xboxReport->Triggers <= midpoint) {
        right = static_cast<uint8_t>((midpoint - xboxReport->Triggers) >> 7);
    } else {
        right = 0;
    }

    // Calculate the left trigger value as a difference from midpoint
    if (xboxReport->Triggers >= midpoint) {
        left = static_cast<uint8_t>((xboxReport->Triggers - midpoint) >> 7);
    } else {
        left = 0;
    }

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputID_GamepadLeftTrigger,
        .InputType = ElemInputType_Analog,
        .Value = NormalizeInputValue(left, 255, 0.1f),
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputID_GamepadRightTrigger,
        .InputType = ElemInputType_Analog,
        .Value = NormalizeInputValue(right, 255, 0.1f),
        .ElapsedSeconds = elapsedSeconds
    });

    // TODO: For buttons we need to keep track of old report to see if there was a change
    // Maybe we can do the same as with delta inputs!



    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputID_GamepadButtonA,
        .InputType = ElemInputType_Digital,
        .Value = (xboxReport->Buttons & 0x01) ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputID_GamepadLeftShoulder,
        .InputType = ElemInputType_Digital,
        .Value = (xboxReport->Buttons & 0x10) ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputID_GamepadRightShoulder,
        .InputType = ElemInputType_Digital,
        .Value = (xboxReport->Buttons & 0x20) ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });
}

HidGamepadHandler HidGamepadHandlers[] =
{
    { .Vendor = HidGamepadVendor_Microsoft, .Product = HidGamepadProduct_XboxOneWirelessOldDriver, .ProcessDataHandler = ProcessXboxOneWirelessOldDriverGamepadReport },
    { .Vendor = HidGamepadVendor_Microsoft, .Product = HidGamepadProduct_XboxOneUsb, .ProcessDataHandler = ProcessXboxOneWirelessOldDriverGamepadReport },
    { .Vendor = HidGamepadVendor_Microsoft, .Product = HidGamepadProduct_XboxOneWireless, .ProcessDataHandler = ProcessXboxOneWirelessGamepadReport }
};

bool IsHidDeviceSupported(uint32_t vendorId, uint32_t productId)
{
    for (uint32_t i = 0; i < ARRAYSIZE(HidGamepadHandlers); i++)
    {
        auto handler = HidGamepadHandlers[i];

        if (vendorId == handler.Vendor && productId == handler.Product)
        {
            return true;
        }
    }

    return false;
}
void ProcessHidDeviceData(ElemWindow window, ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> hidReport, double elapsedSeconds)
{
    auto inputDeviceData = GetInputDeviceData(inputDevice);
    SystemAssert(inputDeviceData);

    for (uint32_t i = 0; i < ARRAYSIZE(HidGamepadHandlers); i++)
    {
        auto handler = HidGamepadHandlers[i];

        if (inputDeviceData->HidVendorId == handler.Vendor && inputDeviceData->HidProductId == handler.Product)
        {
            handler.ProcessDataHandler(window, inputDevice, hidReport, elapsedSeconds);
            return;
        }
    }
}
