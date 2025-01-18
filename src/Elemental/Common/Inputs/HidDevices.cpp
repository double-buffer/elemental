#include "HidDevices.h"
#include "HidUtils.h"
#include "Inputs.h"

#include "HidDualSenseGamepad.cpp"
#include "HidXboxOneGamepad.cpp"
#include "HidSwitchGamepad.cpp"

CheckHidGamepadSupportPtr HidGamepadDeviceModules[] =
{
    CheckHidXboxOneGamepadSupport,
    CheckHidSwitchProGamepadSupport,
    CheckHidDualSenseGamepadSupport
};

bool IsHidDeviceSupported(uint32_t vendorId, uint32_t productId)
{
    for (uint32_t i = 0; i < ARRAYSIZE(HidGamepadDeviceModules); i++)
    {
        auto handler = HidGamepadDeviceModules[i](vendorId, productId);

        if (handler)
        {
            return true;
        }
    }

    return false;
}

void ProcessHidDeviceData(ElemWindow window, ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> hidReport, double elapsedSeconds)
{
    auto inputDeviceData = GetInputDeviceData(inputDevice);
    SystemAssert(inputDeviceData);
    
    if (inputDeviceData->HidDeviceHandler == nullptr)
    {
        for (uint32_t i = 0; i < ARRAYSIZE(HidGamepadDeviceModules); i++)
        {
            inputDeviceData->HidDeviceHandler = (void*)HidGamepadDeviceModules[i](inputDeviceData->HidVendorId, inputDeviceData->HidProductId);

            if (inputDeviceData->HidDeviceHandler)
            {
                break;
            }
        }
    }

    SystemAssert(inputDeviceData->HidDeviceHandler);
    ((ProcessHidGamepadInputReportPtr)inputDeviceData->HidDeviceHandler)(window, inputDevice, hidReport, elapsedSeconds);
}
