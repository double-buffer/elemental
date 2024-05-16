#pragma once

#include "../Elemental.h"

#define MAX_INPUT_DEVICES 64u
#define MAX_INPUT_EVENTS 1024u

struct InputDeviceData
{
    void* PlatformHandle;
    ElemInputDeviceType InputDeviceType;
    uint32_t HidVendorId;
    uint32_t HidProductId;
};

struct InputDeviceDataFull
{
    uint32_t reserved;
};

InputDeviceData* GetInputDeviceData(ElemInputDevice inputDevice);
InputDeviceDataFull* GetInputDeviceDataFull(ElemInputDevice inputDevice);

ElemInputDevice AddInputDevice(InputDeviceData* deviceData, InputDeviceDataFull* deviceDataFull);
void AddInputEvent(ElemInputEvent inputEvent);
