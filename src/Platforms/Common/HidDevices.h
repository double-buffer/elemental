#pragma once
#include <stdint.h>
#include <stdlib.h>
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

float NormalizeInputSigned(uint32_t value, uint32_t maxValue)
{
    return ((float)value / maxValue) * 2.0f - 1.0f;
}

void SetInputObjectAnalogValue(struct InputState inputState, enum InputObjectKey inputObjectKey, float value) 
{
    struct InputObject inputObject = ((struct InputObject*)inputState.InputObjectsPointer)[inputObjectKey];
    ((float*)inputState.DataPointer)[inputObject.Value.Offset] = value;
}

void ConvertHidInputDeviceData_XboxOneWirelessGamepad(struct InputState inputState, int gamepadIndex, void* reportData, uint32_t reportSizeInBytes)
{
    struct XboxOneWirelessGamepadReport* inputData = (struct XboxOneWirelessGamepadReport*)reportData;

    SetInputObjectAnalogValue(inputState, Gamepad1LeftStickX, NormalizeInputSigned(inputData->LeftStickX, 65535.0f));
    SetInputObjectAnalogValue(inputState, Gamepad1LeftStickY, NormalizeInputSigned(inputData->LeftStickY, 65535.0f));
}

static uint32_t* globalInputStateData = 0;
static int32_t globalInputStateDataSize = 0;
static int32_t inputStateCurrentAllocatedIndex = 0;
static struct InputObject* globalInputObjects = 0;
static int32_t globalInputObjectsSize = 0;

void CreateInputObject(enum InputObjectKey inputObjectKey, enum InputObjectType inputObjectType)
{
    globalInputObjects[inputObjectKey].Type = inputObjectType;
    globalInputObjects[inputObjectKey].Value.Offset = inputStateCurrentAllocatedIndex++;
}

void InitGamepad(int32_t gamePadIndex)
{
    CreateInputObject(Gamepad1LeftStickX, InputObjectType_Analog);
    CreateInputObject(Gamepad1LeftStickY, InputObjectType_Analog);
}

struct InputState InitInputState()
{
    globalInputObjectsSize = InputObjectKey_MaxValue + 1;
    globalInputObjects = (struct InputObject*)calloc(globalInputObjectsSize, sizeof(struct InputObject));
    
    // TODO: Refine that for now we allocate the maximum amout of data
    // TODO: Do a 2 pass approach
    globalInputStateDataSize = globalInputObjectsSize;
    globalInputStateData = (uint32_t*)calloc(globalInputStateDataSize, sizeof(uint32_t));

    InitGamepad(0);

    struct InputState result;
    result.DataPointer = globalInputStateData;
    result.DataSize = globalInputStateDataSize;
    result.InputObjectsPointer = globalInputObjects;
    result.InputObjectsSize = globalInputObjectsSize;

    return result;
}

void FreeInputState(struct InputState inputState)
{
    free(inputState.DataPointer);
    free(inputState.InputObjectsPointer);
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