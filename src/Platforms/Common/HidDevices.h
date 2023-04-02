#pragma once
#include <stdint.h>
#include "Elemental.h"

#define PackedStruct __attribute__((__packed__))

enum HidGamepadVendor: uint32_t 
{
    HidGamepadVendor_Microsoft = 0x045E,
    HidGamepadVendor_Sony = 0x054C
};

enum HidGamepadProduct: uint32_t 
{
    HidGamepadProduct_XboxOneWireless = 0x02FD,
    HidGamepadProduct_DualShock4 = 0x5C4
};

// HID report struct for Xbox One Wireless Gamepad
struct PackedStruct XboxOneWirelessGamepadReport 
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

void SetInputObjectAnalogValue(struct InputState inputState, enum InputObjectKey inputObjectKey, float value) 
{
    struct InputObject inputObject = ((struct InputObject*)inputState.InputObjectsPointer)[inputObjectKey];
    ((float*)inputState.DataPointer)[inputObject.Value.Offset] = value;
}

void ConvertHidInputDeviceData_XboxOneWirelessGamepad(struct InputState inputState, int gamepadIndex, void* reportData, uint32_t reportSizeInBytes)
{
    struct XboxOneWirelessGamepadReport* inputData = (struct XboxOneWirelessGamepadReport*)reportData;

    SetInputObjectAnalogValue(inputState, Gamepad1LeftStickX, ((float)inputData->LeftStickX / 65535.0f) * 2.0f - 1.0f);
}

typedef void (*ConvertHidInputDeviceDataFuncPtr)(struct InputState inputState, int gamepadIndex, void* reportData, uint32_t reportSizeInBytes);

ConvertHidInputDeviceDataFuncPtr GetConvertHidInputDeviceDataFuncPtr(uint32_t vendorId, uint32_t productId)
{
    if (vendorId == HidGamepadVendor_Microsoft && productId == HidGamepadProduct_XboxOneWireless)
    {
        return &ConvertHidInputDeviceData_XboxOneWirelessGamepad;
    }

    return 0;
}