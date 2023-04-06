#pragma once
#include <stdint.h>
#include <stdlib.h>
#include "SystemFunctions.h"
#include "Elemental.h"

float NormalizeInputSigned(uint32_t value, uint32_t maxValue)
{
    // TODO: Allows the configuration of deadzone
    float_t deadZone = 0.25f;
    float_t normalizedValue = ((float)value / maxValue) * 2.0f - 1.0f;

    if (normalizedValue < deadZone && normalizedValue > -deadZone)
    {
        return 0.0f;
    }

    return normalizedValue;
}

void SetInputObjectAnalogValue(struct InputState inputState, enum InputObjectKey inputObjectKey, float_t value) 
{
    struct InputObject inputObject = ((struct InputObject*)inputState.InputObjectsPointer)[inputObjectKey];
    ((float_t*)inputState.DataPointer)[inputObject.Value.Offset] = value;
}

void SetInputObjectDigitalValue(struct InputState inputState, enum InputObjectKey inputObjectKey, bool value) 
{
    struct InputObject inputObject = ((struct InputObject*)inputState.InputObjectsPointer)[inputObjectKey];

    uint32_t currentValue = ((uint32_t *)inputState.DataPointer)[inputObject.Value.Offset];
    uint32_t mask = 1 << inputObject.Value.BitPosition;
    ((uint32_t *)inputState.DataPointer)[inputObject.Value.Offset] = (currentValue & ~mask) | ((value ? 1 : 0) << inputObject.Value.BitPosition);
}

//---------------------------------------------------------------------------------------------------------------
// Vendor specific gamepad code
//---------------------------------------------------------------------------------------------------------------

enum HidGamepadVendor: uint32_t 
{
    HidGamepadVendor_Microsoft = 0x045E,
    HidGamepadVendor_Sony = 0x054C
};

enum HidGamepadProduct: uint32_t 
{
    HidGamepadProduct_XboxOneWirelessOldDriver = 0x02E0,
    HidGamepadProduct_XboxOneWireless = 0x02FD,
    HidGamepadProduct_DualShock4 = 0x5C4
};

//---------------------------------------------------------------------------------------------------------------
// Xbox One Wireless old driver
//---------------------------------------------------------------------------------------------------------------

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
PackedStructEnd

void ConvertHidInputDeviceData_XboxOneWirelessOldDriverGamepad(struct InputState inputState, int gamepadIndex, void* reportData, uint32_t reportSizeInBytes)
{
    struct XboxOneWirelessOldDriverGamepadReport* inputData = (struct XboxOneWirelessOldDriverGamepadReport*)reportData;

    SetInputObjectAnalogValue(inputState, Gamepad1LeftStickX, NormalizeInputSigned(inputData->LeftStickX, 65535.0f));
    SetInputObjectAnalogValue(inputState, Gamepad1LeftStickY, -NormalizeInputSigned(inputData->LeftStickY, 65535.0f));
    SetInputObjectDigitalValue(inputState, Gamepad1Button1, inputData->Buttons & 0x01);
    SetInputObjectDigitalValue(inputState, Gamepad1Button2, inputData->Buttons & 0x02);
}

//---------------------------------------------------------------------------------------------------------------
// Xbox One Wireless
//---------------------------------------------------------------------------------------------------------------

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
PackedStructEnd

void ConvertHidInputDeviceData_XboxOneWirelessGamepad(struct InputState inputState, int gamepadIndex, void* reportData, uint32_t reportSizeInBytes)
{
    struct XboxOneWirelessGamepadReport* inputData = (struct XboxOneWirelessGamepadReport*)reportData;

    SetInputObjectAnalogValue(inputState, Gamepad1LeftStickX, NormalizeInputSigned(inputData->LeftStickX, 65535.0f));
    SetInputObjectAnalogValue(inputState, Gamepad1LeftStickY, -NormalizeInputSigned(inputData->LeftStickY, 65535.0f));
    SetInputObjectDigitalValue(inputState, Gamepad1Button1, inputData->Buttons & 0x01);
    SetInputObjectDigitalValue(inputState, Gamepad1Button2, inputData->Buttons & 0x02);
}

//---------------------------------------------------------------------------------------------------------------
// Vendor gamepad dispatcher
//---------------------------------------------------------------------------------------------------------------

typedef void (*ConvertHidInputDeviceDataFuncPtr)(struct InputState inputState, int gamepadIndex, void* reportData, uint32_t reportSizeInBytes);

ConvertHidInputDeviceDataFuncPtr GetConvertHidInputDeviceDataFuncPtr(uint32_t vendorId, uint32_t productId)
{
    if (vendorId == HidGamepadVendor_Microsoft)
    {
        if (productId == HidGamepadProduct_XboxOneWirelessOldDriver)
        {
            return &ConvertHidInputDeviceData_XboxOneWirelessOldDriverGamepad;
        }
        else if (productId == HidGamepadProduct_XboxOneWireless)
        {
            return &ConvertHidInputDeviceData_XboxOneWirelessGamepad;
        }
    }

    return 0;
}

//---------------------------------------------------------------------------------------------------------------
// Input state memory management
//---------------------------------------------------------------------------------------------------------------

static uint32_t* globalInputStateData = 0;
static int32_t globalInputStateDataSize = 0;
static int32_t inputStateCurrentAllocatedIndex = 0;
static uint8_t inputStateCurrentAllocatedBitPosition = 0;
static bool currentAllocationBits = false;
static struct InputObject* globalInputObjects = 0;
static int32_t globalInputObjectsSize = 0;

void CreateInputObject(enum InputObjectKey inputObjectKey, enum InputObjectType inputObjectType)
{
    globalInputObjects[inputObjectKey].Type = inputObjectType;

    if (inputObjectType == InputObjectType_Analog)
    {
        globalInputObjects[inputObjectKey].Value.Offset = inputStateCurrentAllocatedIndex++;
        globalInputObjects[inputObjectKey].Value.BitPosition = 0;
        currentAllocationBits = false;
    }
    else 
    {
        if (!currentAllocationBits || inputStateCurrentAllocatedBitPosition == 32)
        {
            inputStateCurrentAllocatedBitPosition = 0;
        }

        globalInputObjects[inputObjectKey].Value.Offset = inputStateCurrentAllocatedIndex;
        globalInputObjects[inputObjectKey].Value.BitPosition = inputStateCurrentAllocatedBitPosition++;
        currentAllocationBits = true;
    }

}

void InitGamepad(int32_t gamePadIndex)
{
    CreateInputObject(Gamepad1LeftStickX, InputObjectType_Analog);
    CreateInputObject(Gamepad1LeftStickY, InputObjectType_Analog);
    CreateInputObject(Gamepad1RightStickX, InputObjectType_Analog);
    CreateInputObject(Gamepad1RightStickY, InputObjectType_Analog);
    CreateInputObject(Gamepad1Button1, InputObjectType_Digital);
    CreateInputObject(Gamepad1Button2, InputObjectType_Digital);
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