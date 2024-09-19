#pragma once

#include "../Elemental.h"
#include "SystemMemory.h"

#define MAX_INPUT_DEVICES 64u
#define MAX_INPUT_EVENTS 1024u

struct InputDeviceData
{
    void* PlatformHandle;
    ElemInputDeviceType InputDeviceType;
    uint32_t HidVendorId;
    uint32_t HidProductId;
    void* PlatformData;
    ElemHandle HidDeviceData;
};

struct InputDeviceDataFull
{
    uint32_t reserved;
};

extern MemoryArena InputsMemoryArena;

InputDeviceData* GetInputDeviceData(ElemInputDevice inputDevice);
InputDeviceDataFull* GetInputDeviceDataFull(ElemInputDevice inputDevice);

ElemInputDevice AddInputDevice(InputDeviceData* deviceData, InputDeviceDataFull* deviceDataFull);
void RemoveInputDevice(ElemInputDevice inputDevice);

void AddInputEvent(ElemInputEvent inputEvent, bool needReset = false);
void ResetInputsFrame();

bool PlatformHidSendOutputReport(ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> data);
bool PlatformHidSendFeatureReport(ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> data);
bool PlatformHidGetFeatureReport(ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> data);

