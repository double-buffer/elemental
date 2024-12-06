#include "HidSwitchGamepad.cpp"
#include "HidDualSenseGamepad.cpp"

// TODO: Get rid of the headers here. We should only include the cpp files
#include "HidDevices.h"
#include "HidUtils.h"
#include "Inputs.h"
#include "SystemFunctions.h"

enum HidGamepadVendor : uint32_t 
{
    HidGamepadVendor_Microsoft = 0x045E,
    HidGamepadVendor_Sony = 0x054C,
    HidGamepadVendor_Nintendo = 0x057E
};

enum HidGamepadProduct : uint32_t 
{
    HidGamepadProduct_XboxOneWirelessOldDriver = 0x02E0,
    HidGamepadProduct_XboxOneUsb = 0x02FF,
    HidGamepadProduct_XboxOneWireless = 0x02FD,
    HidGamepadProduct_DualShock4OldDriver = 0x5C4,
    HidGamepadProduct_DualShock4 = 0x9cc,
    HidGamepadProduct_DualSense = 0x0ce6,
    HidGamepadProduct_SwitchPro = 0x2009
};

typedef void (*ProcessHidGamepadInputReportPtr)(ElemWindow window, ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> hidReport, double elapsedSeconds);

struct HidGamepadHandler
{
    HidGamepadVendor Vendor;
    HidGamepadProduct Product;
    ProcessHidGamepadInputReportPtr ProcessDataHandler;
};

struct __attribute__((__packed__)) XboxOneWirelessOldDriverGamepadReport
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

struct __attribute__((__packed__)) XboxOneWirelessGamepadReport
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

struct __attribute__((__packed__)) DualSenseSimpleGamepadReport
{
    uint8_t LeftStickX;
    uint8_t LeftStickY;
    uint16_t RightStickX;
    uint16_t RightStickY;
    uint16_t LeftTrigger;
    uint16_t RightTrigger;
    uint8_t Dpad;
    uint32_t Buttons;
};

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
        .InputId = ElemInputId_GamepadLeftTrigger,
        .InputType = ElemInputType_Analog,
        .Value = NormalizeInputValue(xboxReport->LeftTrigger, 1024, 0.1f),
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputId_GamepadRightTrigger,
        .InputType = ElemInputType_Analog,
        .Value = NormalizeInputValue(xboxReport->RightTrigger, 1024, 0.1f),
        .ElapsedSeconds = elapsedSeconds
    });

    // TODO: For buttons we need to keep track of old report to see if there was a change
    // Maybe we can do the same as with delta inputs!

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputId_GamepadButtonA,
        .InputType = ElemInputType_Digital,
        .Value = (xboxReport->Buttons & 0x01) ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });
    
    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputId_GamepadButtonB,
        .InputType = ElemInputType_Digital,
        .Value = (xboxReport->Buttons & 0x02) ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputId_GamepadLeftShoulder,
        .InputType = ElemInputType_Digital,
        .Value = (xboxReport->Buttons & 0x40) ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputId_GamepadRightShoulder,
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
        .InputId = ElemInputId_GamepadLeftTrigger,
        .InputType = ElemInputType_Analog,
        .Value = NormalizeInputValue(left, 255, 0.1f),
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputId_GamepadRightTrigger,
        .InputType = ElemInputType_Analog,
        .Value = NormalizeInputValue(right, 255, 0.1f),
        .ElapsedSeconds = elapsedSeconds
    });

    // TODO: For buttons we need to keep track of old report to see if there was a change
    // Maybe we can do the same as with delta inputs!



    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputId_GamepadButtonA,
        .InputType = ElemInputType_Digital,
        .Value = (xboxReport->Buttons & 0x01) ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputId_GamepadButtonB,
        .InputType = ElemInputType_Digital,
        .Value = (xboxReport->Buttons & 0x02) ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputId_GamepadLeftShoulder,
        .InputType = ElemInputType_Digital,
        .Value = (xboxReport->Buttons & 0x10) ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = ElemInputId_GamepadRightShoulder,
        .InputType = ElemInputType_Digital,
        .Value = (xboxReport->Buttons & 0x20) ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });
}

HidGamepadHandler HidGamepadHandlers[] =
{
    { .Vendor = HidGamepadVendor_Microsoft, .Product = HidGamepadProduct_XboxOneWirelessOldDriver, .ProcessDataHandler = ProcessXboxOneWirelessOldDriverGamepadReport },
    { .Vendor = HidGamepadVendor_Microsoft, .Product = HidGamepadProduct_XboxOneUsb, .ProcessDataHandler = ProcessXboxOneWirelessOldDriverGamepadReport },
    { .Vendor = HidGamepadVendor_Microsoft, .Product = HidGamepadProduct_XboxOneWireless, .ProcessDataHandler = ProcessXboxOneWirelessGamepadReport },
    { .Vendor = HidGamepadVendor_Sony, .Product = HidGamepadProduct_DualSense, .ProcessDataHandler = ProcessHidDualSenseGamepadInputReport },
    { .Vendor = HidGamepadVendor_Nintendo, .Product = HidGamepadProduct_SwitchPro, .ProcessDataHandler = ProcessHidSwitchGamepadInputReport }
};

// TODO: Allow each providers to handle registration
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
