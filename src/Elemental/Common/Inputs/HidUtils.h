#pragma once

#include "../Elemental.h"
#include "Inputs.h"

typedef void (*ProcessHidGamepadInputReportPtr)(ElemWindow window, ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> hidReport, double elapsedSeconds);
typedef ProcessHidGamepadInputReportPtr (*CheckHidGamepadSupportPtr)(uint32_t vendorId, uint32_t productId);

// TODO: Create a CPP file
// TODO: Rename that file

inline float NormalizeInputValue(uint32_t value, uint32_t maxValue, float deadZone)
{
    // TODO: Allows the configuration of deadzone
    float normalizedValue = ((float)value / (float)maxValue);

    if (normalizedValue < deadZone)
    {
        return 0.0f;
    }

    return normalizedValue;
}

inline float NormalizeInputValueSigned(uint32_t value, uint32_t maxValue, float deadZone)
{
    // TODO: Allows the configuration of deadzone
    float normalizedValue = ((float)value / (float)maxValue) * 2.0f - 1.0f;

    if (normalizedValue < deadZone && normalizedValue > -deadZone)
    {
        return 0.0f;
    }

    return normalizedValue;
}

inline void ProcessHidGamepadDeltaAxe(ElemWindow window, ElemInputDevice inputDevice, double elapsedSeconds, float axeData, ElemInputId negativeInputId, ElemInputId positiveInputId)
{
    if (axeData < 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = negativeInputId,
            .InputType = ElemInputType_Delta,
            .Value = -axeData,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (axeData > 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = positiveInputId,
            .InputType = ElemInputType_Delta,
            .Value = axeData,
            .ElapsedSeconds = elapsedSeconds
        });
    }
}

inline void ProcessHidGamepadStick(ElemWindow window, ElemInputDevice inputDevice, double elapsedSeconds, float stickData, float previousStickData, ElemInputId negativeInputId, ElemInputId positiveInputId)
{
    if (stickData != previousStickData)
    {
        if (stickData <= 0)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = negativeInputId,
                .InputType = ElemInputType_Analog,
                .Value = -stickData,
                .ElapsedSeconds = elapsedSeconds
            });
        }

        if (stickData >= 0)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = positiveInputId,
                .InputType = ElemInputType_Analog,
                .Value = stickData,
                .ElapsedSeconds = elapsedSeconds
            });
        }
    }
}

inline void ProcessHidGamepadTrigger(ElemWindow window, ElemInputDevice inputDevice, double elapsedSeconds, float data, float previousData, ElemInputId inputId)
{
    if (data != previousData)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = inputId,
            .InputType = ElemInputType_Analog,
            .Value = data,
            .ElapsedSeconds = elapsedSeconds
        });
    }
}

inline void ProcessHidGamepadButton(ElemWindow window, ElemInputDevice inputDevice, double elapsedSeconds, uint16_t currentButtons, uint16_t previousButtons, uint16_t buttonType, ElemInputId inputId)
{
    if ((currentButtons & buttonType) != (previousButtons & buttonType))
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = inputId,
            .InputType = ElemInputType_Digital,
            .Value = (currentButtons & buttonType) ? 1.0f : 0.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }
}
