#pragma once
#include "SystemFunctions.h"
#include "Elemental.h"

float NormalizeInputSigned(uint32_t value, uint32_t maxValue)
{
    // TODO: Allows the configuration of deadzone
    float_t deadZone = 0.25f;
    float_t normalizedValue = ((float_t)value / (float_t)maxValue) * 2.0f - 1.0f;

    if (normalizedValue < deadZone && normalizedValue > -deadZone)
    {
        return 0.0f;
    }

    return normalizedValue;
}

void SetInputObjectAnalogValue(InputState* inputState, enum InputObjectKey inputObjectKey, float_t value) 
{
    if (inputState->DataPointer == NULL)
    {
        return;
    }

    InputObject inputObject = ((InputObject*)inputState->InputObjectsPointer)[inputObjectKey];
    ((float_t*)inputState->DataPointer)[inputObject.Value.Offset] = value;
}

void SetInputObjectDigitalValue(InputState* inputState, enum InputObjectKey inputObjectKey, bool value) 
{
    if (inputState->DataPointer == NULL)
    {
        return;
    }

    InputObject inputObject = ((InputObject*)inputState->InputObjectsPointer)[inputObjectKey];

    uint32_t currentValue = ((uint32_t *)inputState->DataPointer)[inputObject.Value.Offset];
    uint32_t mask = 1u << (uint32_t)inputObject.Value.BitPosition;
    ((uint32_t *)inputState->DataPointer)[inputObject.Value.Offset] = (currentValue & ~mask) | ((value ? 1 : 0) << inputObject.Value.BitPosition);
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

typedef PackedStruct
{
    uint8_t Padding;
    uint16_t LeftStickX;
    uint16_t LeftStickY;
    uint16_t RightStickX;
    uint16_t RightStickY;
    uint16_t Triggers;
    uint16_t Buttons;
    uint8_t Dpad;
} XboxOneWirelessOldDriverGamepadReport;
PackedStructEnd

void ConvertHidInputDeviceData_XboxOneWirelessOldDriverGamepad(InputState* inputState, int gamepadIndex, void* reportData, uint32_t reportSizeInBytes)
{
    XboxOneWirelessOldDriverGamepadReport* inputData = (XboxOneWirelessOldDriverGamepadReport*)reportData;

    SetInputObjectAnalogValue(inputState, Gamepad1LeftStickX, NormalizeInputSigned(inputData->LeftStickX, 65535));
    SetInputObjectAnalogValue(inputState, Gamepad1LeftStickY, -NormalizeInputSigned(inputData->LeftStickY, 65535));
    SetInputObjectDigitalValue(inputState, Gamepad1Button1, inputData->Buttons & 0x01);
    SetInputObjectDigitalValue(inputState, Gamepad1Button2, inputData->Buttons & 0x02);
}

//---------------------------------------------------------------------------------------------------------------
// Xbox One Wireless
//---------------------------------------------------------------------------------------------------------------

typedef PackedStruct
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
} XboxOneWirelessGamepadReport;
PackedStructEnd

void ConvertHidInputDeviceData_XboxOneWirelessGamepad(InputState* inputState, int gamepadIndex, void* reportData, uint32_t reportSizeInBytes)
{
    XboxOneWirelessGamepadReport* inputData = (XboxOneWirelessGamepadReport*)reportData;

    SetInputObjectAnalogValue(inputState, Gamepad1LeftStickX, NormalizeInputSigned(inputData->LeftStickX, 65535));
    SetInputObjectAnalogValue(inputState, Gamepad1LeftStickY, -NormalizeInputSigned(inputData->LeftStickY, 65535));
    SetInputObjectDigitalValue(inputState, Gamepad1Button1, inputData->Buttons & 0x01);
    SetInputObjectDigitalValue(inputState, Gamepad1Button2, inputData->Buttons & 0x02);
}

//---------------------------------------------------------------------------------------------------------------
// Vendor gamepad dispatcher
//---------------------------------------------------------------------------------------------------------------

typedef void (*ConvertHidInputDeviceDataFuncPtr)(InputState* inputState, int gamepadIndex, void* reportData, uint32_t reportSizeInBytes);

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
static size_t globalInputStateDataSize = 0;
static uint32_t inputStateCurrentAllocatedIndex = 0;
static uint8_t inputStateCurrentAllocatedBitPosition = 0;
static bool currentAllocationBits = false;
static InputObject* globalInputObjects = 0;
static size_t globalInputObjectsSize = 0;

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

// BUG: It seems that sometimes the state is not init on MacOS. We have random gamepad movement
InputState* InitInputState()
{
    globalInputObjectsSize = InputObjectKey_MaxValue + 1;
    globalInputObjects = (InputObject*)calloc(globalInputObjectsSize, sizeof(InputObject));
    
    // TODO: Refine that for now we allocate the maximum amout of data
    // TODO: Do a 2 pass approach
    globalInputStateDataSize = globalInputObjectsSize;
    globalInputStateData = (uint32_t*)calloc(globalInputStateDataSize, sizeof(uint32_t));

    InitGamepad(0);

    InputState* result = (InputState*)malloc(sizeof(InputState));
    result->DataPointer = globalInputStateData;
    result->DataSize = globalInputStateDataSize;
    result->InputObjectsPointer = globalInputObjects;
    result->InputObjectsSize = globalInputObjectsSize;

    return result;
}

void FreeInputState(InputState* inputState)
{
    free(inputState->DataPointer);
    inputState->DataPointer = NULL;

    free(inputState->InputObjectsPointer);
    inputState->InputObjectsPointer = NULL;
}