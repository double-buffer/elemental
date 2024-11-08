#pragma once

#include "../Elemental.h"
#include "Inputs.h"
#include "HidUtils.h"
#include "SystemFunctions.h"
#include "SystemDataPool.h"

// Based on the spec: https://controllers.fandom.com/wiki/Sony_DualSense

#define HID_DUALSENSE_STICK_RANGE 255
#define HID_DUALSENSE_STICK_DEAD_ZONE 0.1f

enum HidDualSenseGamepadInputReportType
{
    HidDualSenseGamepadInputReportType_BluetoothStandard = 0x01,
    //HidDualSenseGamepadInputReportType_SubCommand = 0x21
};

enum HidDualSenseGamepadOutputReportType
{
    //HidDualSenseGamepadOutputReportType_SubCommand = 0x01
};

/*
enum HidDualSenseGamepadSubCommandType : uint8_t
{
    HidDualSenseGamepadSubCommandType_DeviceInfo = 0x02
};

enum HidDualSenseGamepadControllerType : uint8_t
{
    HidDualSenseGamepadControllerType_LeftJoyCon = 0x01,  
    HidDualSenseGamepadControllerType_RightJoyCon = 0x02,  
    HidDualSenseGamepadControllerType_ProController = 0x03,  
};*/

enum HidDualSenseGamepadButton : uint16_t
{
    HidDualSenseGamepadButton_Square = 1 << 4,
    HidDualSenseGamepadButton_Cross = 1 << 5,
    HidDualSenseGamepadButton_Circle = 1 << 6,
    HidDualSenseGamepadButton_Triangle = 1 << 7,
    HidDualSenseGamepadButton_L1 = 1 << 8,
    HidDualSenseGamepadButton_R1 = 1 << 9,
    HidDualSenseGamepadButton_L2 = 1 << 10,
    HidDualSenseGamepadButton_R2 = 1 << 11,
    HidDualSenseGamepadButton_Share = 1 << 12,
    HidDualSenseGamepadButton_Options = 1 << 13,
    HidDualSenseGamepadButton_L3 = 1 << 14,
    HidDualSenseGamepadButton_R3 = 1 << 15
};

struct __attribute__((__packed__)) HidDualSenseGamepadInputReportBluetoothStandard
{
    uint8_t LeftStickX;
    uint8_t LeftStickY;
    uint8_t RightStickX;
    uint8_t RightStickY;
    uint16_t Buttons;
    uint8_t SystemButtons;
    uint8_t LeftTrigger;
    uint8_t RightTrigger;
};

struct __attribute__((__packed__)) HidDualSenseGamepadInputReportExtended
{
    uint8_t Timer;
    uint8_t BatteryConnection;
    uint8_t Buttons[3];
    uint8_t LeftStick[3];
    uint8_t RightStick[3];
};

struct HidDualSenseGamepadData
{
    float PreviousLeftStickX;
    float PreviousLeftStickY;
    uint16_t PreviousButtons;
};

/*
struct __attribute__((__packed__)) HidDualSenseGamepadInputReportDeviceInfo
{
    uint16_t FirmwareVersion;
    HidDualSenseGamepadControllerType ControllerType;
};

struct __attribute__((__packed__)) HidDualSenseGamepadOutputReportSubCommand
{
    uint8_t ReportId;
    uint8_t GlobalPacketNumber;
    // TODO: Rumble Data
    uint8_t RumbleData[8];
    HidDualSenseGamepadSubCommandType SubCommandId;
    uint8_t Data[53];
};

struct __attribute__((__packed__)) HidDualSenseGamepadFeatureReportSetupMemoryRead
{
    uint8_t ReportId;
    uint32_t Address;
    uint16_t Size;
    uint8_t Checksum;
};

struct __attribute__((__packed__)) HidDualSenseGamepadFeatureReportMemoryRead
{
    uint8_t ReportId;
    uint8_t Checksum;
    uint8_t Data[1024];
};*/

static SystemDataPool<HidDualSenseGamepadData, SystemDataPoolDefaultFull> hidDualSenseGamepadDataPool;

inline void InitHidDualSenseGamepadMemory()
{
    if (hidDualSenseGamepadDataPool.Storage == nullptr)
    {
        hidDualSenseGamepadDataPool = SystemCreateDataPool<HidDualSenseGamepadData>(InputsMemoryArena, MAX_INPUT_DEVICES);
    }
}

inline HidDualSenseGamepadData* GetHidDualSenseGamepadData(ElemHandle hidDevice)
{
    return SystemGetDataPoolItem(hidDualSenseGamepadDataPool, hidDevice);
}

/*
inline bool SendHidDualSenseGamepadSubCommand(ElemInputDevice inputDevice, HidDualSenseGamepadSubCommandType subCommandType, ReadOnlySpan<uint8_t> data)
{
    auto inputDeviceData = GetInputDeviceData(inputDevice);
    SystemAssert(inputDeviceData != ELEM_HANDLE_NULL);

    auto hidData = GetHidDualSenseGamepadData(inputDeviceData->HidDeviceData);
    SystemAssert(hidData);

    HidDualSenseGamepadOutputReportSubCommand outputReport =
    {
        .ReportId = HidDualSenseGamepadOutputReportType_SubCommand,
        .GlobalPacketNumber = hidData->CurrentOutputPacketNumber,
        .SubCommandId = subCommandType
    };

    hidData->CurrentOutputPacketNumber = (hidData->CurrentOutputPacketNumber + 1) % 255;

    return PlatformHidSendOutputReport(inputDevice, ReadOnlySpan<uint8_t>((uint8_t*)&outputReport, sizeof(outputReport)));
}*/

inline void ProcessHidDualSenseGamepadStick(ElemWindow window, ElemInputDevice inputDevice, double elapsedSeconds, float stickData, float previousStickData, ElemInputId negativeInputId, ElemInputId positiveInputId)
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

inline void ProcessHidDualSenseGamepadButton(ElemWindow window, ElemInputDevice inputDevice, double elapsedSeconds, uint16_t currentButtons, uint16_t previousButtons, HidDualSenseGamepadButton buttonType, ElemInputId inputId)
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

inline void ProcessHidDualSenseGamepadInputReportBluetoothStandard(ElemWindow window, ElemInputDevice inputDevice, HidDualSenseGamepadInputReportBluetoothStandard* inputReport, double elapsedSeconds)
{
    //SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "DualSenseGamePad Bluetooth Standard Report %f", elapsedSeconds);

    auto inputDeviceData = GetInputDeviceData(inputDevice);
    SystemAssert(inputDeviceData != ELEM_HANDLE_NULL);

    if (inputDeviceData->HidDeviceData != ELEM_HANDLE_NULL)
    {
        auto hidData = GetHidDualSenseGamepadData(inputDeviceData->HidDeviceData);
        SystemAssert(hidData);

        auto leftStickX = NormalizeInputValueSigned(inputReport->LeftStickX, HID_DUALSENSE_STICK_RANGE, HID_DUALSENSE_STICK_DEAD_ZONE);
        ProcessHidDualSenseGamepadStick(window, inputDevice, elapsedSeconds, leftStickX, hidData->PreviousLeftStickX, ElemInputId_GamepadLeftStickXNegative, ElemInputId_GamepadLeftStickXPositive);

        auto leftStickY = -NormalizeInputValueSigned(inputReport->LeftStickY, HID_DUALSENSE_STICK_RANGE, HID_DUALSENSE_STICK_DEAD_ZONE);
        ProcessHidDualSenseGamepadStick(window, inputDevice, elapsedSeconds, leftStickY, hidData->PreviousLeftStickY, ElemInputId_GamepadLeftStickYNegative, ElemInputId_GamepadLeftStickYPositive);

        ProcessHidDualSenseGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_Cross, ElemInputID_GamepadButtonA);
        ProcessHidDualSenseGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_Circle, ElemInputID_GamepadButtonB);
        ProcessHidDualSenseGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_Square, ElemInputID_GamepadButtonX);
        ProcessHidDualSenseGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_Triangle, ElemInputID_GamepadButtonY);
        ProcessHidDualSenseGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_L1, ElemInputID_GamepadLeftShoulder);
        ProcessHidDualSenseGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_R1, ElemInputID_GamepadRightShoulder);
        ProcessHidDualSenseGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_Share, ElemInputID_GamepadButtonShare);
        ProcessHidDualSenseGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_Options, ElemInputID_GamepadButtonOptions);
        ProcessHidDualSenseGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_L3, ElemInputId_GamepadLeftStickButton);
        ProcessHidDualSenseGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_R3, ElemInputId_GamepadRightStickButton);

        // TODO: TESTING ONLY
        if (inputReport->Buttons & 0x01)
        {
            //SendHidDualSenseGamepadSubCommand(inputDevice, HidDualSenseGamepadSubCommandType_DeviceInfo, ReadOnlySpan<uint8_t>());
        }

        hidData->PreviousLeftStickX = leftStickX;
        hidData->PreviousLeftStickY = leftStickY;
        hidData->PreviousButtons = inputReport->Buttons;
    }
}

inline void ProcessHidDualSenseGamepadInputReport(ElemWindow window, ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> hidReport, double elapsedSeconds)
{
    // TODO: We need to differentiate USB / Bluetooth? How can we do that?

    InitHidDualSenseGamepadMemory();

    auto inputDeviceData = GetInputDeviceData(inputDevice);
    SystemAssert(inputDeviceData != ELEM_HANDLE_NULL);

    // TODO: This part is to get the calibration data
    if (inputDeviceData->HidDeviceData == ELEM_HANDLE_NULL)
    {
        /*
        // Read Config
        HidDualSenseGamepadFeatureReportSetupMemoryRead featureReport =
        {
            .ReportId = 0x71,
            .Address = 0xF8000000 + 0x6000,
            .Size = 0xA000,
            .Checksum = 0
        };

        auto pointer = (uint8_t*)&featureReport;
        auto sumBytes = 0u;

        for (uint32_t i = 0; i < sizeof(featureReport); i++)
        {
            sumBytes += pointer[i];
        }
        
        featureReport.Checksum = 0x100 - sumBytes;

        auto result = PlatformHidSendFeatureReport(inputDevice, ReadOnlySpan<uint8_t>((uint8_t*)&featureReport, sizeof(featureReport)));
        SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Send Feature Result: %d", result);

        HidDualSenseGamepadFeatureReportMemoryRead readFeatureReport =
        {
            .ReportId = 0x72,
            .Checksum = 0
        };

        result = PlatformHidGetFeatureReport(inputDevice, ReadOnlySpan<uint8_t>((uint8_t*)&readFeatureReport, sizeof(readFeatureReport)));
        SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Get Feature Result: %d", result);*/

        inputDeviceData->HidDeviceData = SystemAddDataPoolItem(hidDualSenseGamepadDataPool, {});
    }

    auto reportId = hidReport[0];
    
    //SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "DualSenseGamePad %d", reportId);

    // TODO: Use the the advanced report
    if (reportId == HidDualSenseGamepadInputReportType_BluetoothStandard)
    {
        auto inputReport = (HidDualSenseGamepadInputReportBluetoothStandard*)(++hidReport.Pointer);
        ProcessHidDualSenseGamepadInputReportBluetoothStandard(window, inputDevice, inputReport, elapsedSeconds);
    }
    /*
    else if (reportId == HidDualSenseGamepadInputReportType_SubCommand)
    {
        auto hidData = GetHidDualSenseGamepadData(inputDeviceData->HidDeviceData);
        SystemAssert(hidData);


        auto inputReport = (HidDualSenseGamepadInputReportExtended*)(++hidReport.Pointer);

        uint8_t *data = inputReport->LeftStick;
        uint16_t stick_horizontal = data[0] | ((data[1] & 0xF) << 8);
        uint16_t stick_vertical = (data[1] >> 4) | (data[2] << 4);

        SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Report: Timer=%d, X=%d, Y=%d", inputReport->Timer, stick_horizontal, stick_vertical);
        //SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "ProcessSubCommandtype: %d, Version: %d, Type: %d", subCommandType, inputReport->FirmwareVersion, inputReport->ControllerType);
    }*/
}
