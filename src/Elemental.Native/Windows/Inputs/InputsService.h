#pragma once

#include "SystemInputs.h"

#include <vector>

#define GAMEPAD_USAGE_PAGE 0x01
#define GAMEPAD_USAGE_ID   0x05

struct HidInputDevice 
{
    uint32_t DeviceId; 
    uint32_t ReadBufferSizeInBytes;
    HANDLE Device;
    HANDLE Event;
    OVERLAPPED Overlapped;
    ConvertHidInputDeviceDataFuncPtr InputDataConvertFunction;
    uint8_t* ReadBuffer;
};
