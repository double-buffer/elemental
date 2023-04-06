#pragma once

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