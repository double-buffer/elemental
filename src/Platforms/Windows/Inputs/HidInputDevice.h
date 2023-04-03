
struct HidInputDevice 
{
    uint32_t DeviceId; 
    HANDLE Device;
    HANDLE Event;
    ConvertHidInputDeviceDataFuncPtr InputDataConvertFunction; 
};