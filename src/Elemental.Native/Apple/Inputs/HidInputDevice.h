#pragma once
struct HidInputDevice 
{
    uint32_t DeviceId;
    IOHIDDeviceRef Device;
    uint32_t ReadBufferSizeInBytes;
    uint8_t* ReadBuffer;
    ConvertHidInputDeviceDataFuncPtr InputDataConvertFunction;
};