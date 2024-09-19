#pragma once

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
