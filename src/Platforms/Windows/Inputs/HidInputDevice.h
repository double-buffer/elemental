#pragma once

struct HidInputDevice 
{
    uint32_t DeviceId; 
    HANDLE Device;
    HANDLE Event;
    OVERLAPPED Overlapped;
    ConvertHidInputDeviceDataFuncPtr InputDataConvertFunction;
    uint8_t* ReadBuffer;
    uint32_t ReadBufferSizeInBytes;
};