#pragma once

#include "SystemFunctions.h"
#include "Elemental.h"

float NormalizeInputSigned(uint32_t value, uint32_t maxValue);
void SetInputObjectAnalogValue(InputState* inputState, InputObjectKey inputObjectKey, float_t value); 
void SetInputObjectDigitalValue(InputState* inputState, InputObjectKey inputObjectKey, bool value); 

//---------------------------------------------------------------------------------------------------------------
// Vendor specific gamepad code
//---------------------------------------------------------------------------------------------------------------

typedef enum : uint32_t 
{
    HidGamepadVendor_Microsoft = 0x045E,
    HidGamepadVendor_Sony = 0x054C
} HidGamepadVendor;

typedef enum : uint32_t 
{
    HidGamepadProduct_XboxOneWirelessOldDriver = 0x02E0,
    HidGamepadProduct_XboxOneWireless = 0x02FD,
    HidGamepadProduct_DualShock4OldDriver = 0x5C4,
    HidGamepadProduct_DualShock4 = 0x9cc
} HidGamepadProduct;


//---------------------------------------------------------------------------------------------------------------
// Vendor gamepad dispatcher
//---------------------------------------------------------------------------------------------------------------

typedef void (*ConvertHidInputDeviceDataFuncPtr)(InputState* inputState, int gamepadIndex, void* reportData, uint32_t reportSizeInBytes);
ConvertHidInputDeviceDataFuncPtr GetConvertHidInputDeviceDataFuncPtr(uint32_t vendorId, uint32_t productId);

void CreateInputObject(InputObjectKey inputObjectKey, InputObjectType inputObjectType);
void InitGamepad(int32_t gamePadIndex);
InputState* InitInputState();
void FreeInputState(InputState* inputState);
