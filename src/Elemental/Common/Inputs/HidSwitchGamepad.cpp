#pragma once

#include "../Elemental.h"
#include "Inputs.h"
#include "HidUtils.h"
#include "SystemFunctions.h"
#include "SystemDataPool.h"

// Based on the spec: https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/bluetooth_hid_notes.md

#define HID_SWITCH_VENDOR_ID 0x057E
#define HID_SWITCH_PRODUCT_ID 0x2009

#define HID_SWITCH_STICK_RANGE 65535
#define HID_SWITCH_STICK_DEAD_ZONE 0.2f

enum HidSwitchGamepadInputReportType
{
    HidSwitchGamepadInputReportType_Standard = 0x3F,
    HidSwitchGamepadInputReportType_SubCommand = 0x21
};

enum HidSwitchGamepadOutputReportType
{
    HidSwitchGamepadOutputReportType_SubCommand = 0x01
};

enum HidSwitchGamepadSubCommandType : uint8_t
{
    HidSwitchGamepadSubCommandType_DeviceInfo = 0x02
};

enum HidSwitchGamepadControllerType : uint8_t
{
    HidSwitchGamepadControllerType_LeftJoyCon = 0x01,  
    HidSwitchGamepadControllerType_RightJoyCon = 0x02,  
    HidSwitchGamepadControllerType_ProController = 0x03,  
};

struct HidSwitchGamepadData
{
    uint8_t CurrentOutputPacketNumber;
};

struct __attribute__((__packed__)) HidSwitchGamepadInputReportStandard
{
    uint8_t Buttons[2];
    uint8_t Dpad;
    uint16_t LeftStickX;
    uint16_t LeftStickY;
    uint16_t RightStickX;
    uint16_t RightStickY;
};

struct __attribute__((__packed__)) HidSwitchGamepadInputReportExtended
{
    uint8_t Timer;
    uint8_t BatteryConnection;
    uint8_t Buttons[3];
    uint8_t LeftStick[3];
    uint8_t RightStick[3];
};

struct __attribute__((__packed__)) HidSwitchGamepadInputReportDeviceInfo
{
    uint16_t FirmwareVersion;
    HidSwitchGamepadControllerType ControllerType;
};

struct __attribute__((__packed__)) HidSwitchGamepadOutputReportSubCommand
{
    uint8_t ReportId;
    uint8_t GlobalPacketNumber;
    // TODO: Rumble Data
    uint8_t RumbleData[8];
    HidSwitchGamepadSubCommandType SubCommandId;
    uint8_t Data[53];
};

struct __attribute__((__packed__)) HidSwitchGamepadFeatureReportSetupMemoryRead
{
    uint8_t ReportId;
    uint32_t Address;
    uint16_t Size;
    uint8_t Checksum;
};

struct __attribute__((__packed__)) HidSwitchGamepadFeatureReportMemoryRead
{
    uint8_t ReportId;
    uint8_t Checksum;
    uint8_t Data[1024];
};

static SystemDataPool<HidSwitchGamepadData, SystemDataPoolDefaultFull> hidSwitchGamepadDataPool;

inline void InitHidSwitchGamepadMemory()
{
    if (hidSwitchGamepadDataPool.Storage == nullptr)
    {
        hidSwitchGamepadDataPool = SystemCreateDataPool<HidSwitchGamepadData>(InputsMemoryArena, MAX_INPUT_DEVICES);
    }
}

inline HidSwitchGamepadData* GetHidSwitchGamepadData(ElemHandle hidDevice)
{
    return SystemGetDataPoolItem(hidSwitchGamepadDataPool, hidDevice);
}

inline bool SendHidSwitchGamepadSubCommand(ElemInputDevice inputDevice, HidSwitchGamepadSubCommandType subCommandType, ReadOnlySpan<uint8_t> data)
{
    auto inputDeviceData = GetInputDeviceData(inputDevice);
    SystemAssert(inputDeviceData != ELEM_HANDLE_NULL);

    auto hidData = GetHidSwitchGamepadData(inputDeviceData->HidDeviceData);
    SystemAssert(hidData);

    HidSwitchGamepadOutputReportSubCommand outputReport =
    {
        .ReportId = HidSwitchGamepadOutputReportType_SubCommand,
        .GlobalPacketNumber = hidData->CurrentOutputPacketNumber,
        .SubCommandId = subCommandType
    };

    hidData->CurrentOutputPacketNumber = (hidData->CurrentOutputPacketNumber + 1) % 255;

    return PlatformHidSendOutputReport(inputDevice, ReadOnlySpan<uint8_t>((uint8_t*)&outputReport, sizeof(outputReport)));
}

inline void ProcessHidSwitchGamepadInputReportStandard(ElemWindow window, ElemInputDevice inputDevice, HidSwitchGamepadInputReportStandard* inputReport, double elapsedSeconds)
{
    // TODO: Also here we need to send the event only if the analog value has changed
    float leftStickX = NormalizeInputValueSigned(inputReport->LeftStickX, HID_SWITCH_STICK_RANGE, HID_SWITCH_STICK_DEAD_ZONE);

    if (leftStickX <= 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_GamepadLeftStickXNegative,
            .InputType = ElemInputType_Analog,
            .Value = -leftStickX,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (leftStickX >= 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_GamepadLeftStickXPositive,
            .InputType = ElemInputType_Analog,
            .Value = leftStickX,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    float leftStickY = -NormalizeInputValueSigned(inputReport->LeftStickY, HID_SWITCH_STICK_RANGE, HID_SWITCH_STICK_DEAD_ZONE);

    if (leftStickY <= 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_GamepadLeftStickYNegative,
            .InputType = ElemInputType_Analog,
            .Value = -leftStickY,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (leftStickY >= 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_GamepadLeftStickYPositive,
            .InputType = ElemInputType_Analog,
            .Value = leftStickY,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    // TODO: TESTING ONLY
    if (inputReport->Buttons[0] & 0x01)
    {
        SendHidSwitchGamepadSubCommand(inputDevice, HidSwitchGamepadSubCommandType_DeviceInfo, ReadOnlySpan<uint8_t>());
    }
}

inline void ProcessHidSwitchGamepadInputReport(ElemWindow window, ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> hidReport, double elapsedSeconds)
{
    InitHidSwitchGamepadMemory();

    auto inputDeviceData = GetInputDeviceData(inputDevice);
    SystemAssert(inputDeviceData != ELEM_HANDLE_NULL);

    if (inputDeviceData->HidDeviceData == ELEM_HANDLE_NULL)
    {
        // Read Config
        HidSwitchGamepadFeatureReportSetupMemoryRead featureReport =
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

        HidSwitchGamepadFeatureReportMemoryRead readFeatureReport =
        {
            .ReportId = 0x72,
            .Checksum = 0
        };

        result = PlatformHidGetFeatureReport(inputDevice, ReadOnlySpan<uint8_t>((uint8_t*)&readFeatureReport, sizeof(readFeatureReport)));
        SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Get Feature Result: %d", result);

        inputDeviceData->HidDeviceData = SystemAddDataPoolItem(hidSwitchGamepadDataPool, {});
    }

    auto reportId = hidReport[0];
    
    //SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "SwitchGamePad %d", reportId);

    // TODO: Use the the advanced report
    if (reportId == HidSwitchGamepadInputReportType_Standard)
    {
        auto inputReport = (HidSwitchGamepadInputReportStandard*)(++hidReport.Pointer);
        ProcessHidSwitchGamepadInputReportStandard(window, inputDevice, inputReport, elapsedSeconds);
    }
    else if (reportId == HidSwitchGamepadInputReportType_SubCommand)
    {
        auto hidData = GetHidSwitchGamepadData(inputDeviceData->HidDeviceData);
        SystemAssert(hidData);


        auto inputReport = (HidSwitchGamepadInputReportExtended*)(++hidReport.Pointer);

        uint8_t *data = inputReport->LeftStick;
        uint16_t stick_horizontal = data[0] | ((data[1] & 0xF) << 8);
        uint16_t stick_vertical = (data[1] >> 4) | (data[2] << 4);

        SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Report: Timer=%d, X=%d, Y=%d", inputReport->Timer, stick_horizontal, stick_vertical);
        //SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "ProcessSubCommandtype: %d, Version: %d, Type: %d", subCommandType, inputReport->FirmwareVersion, inputReport->ControllerType);
    }
}

ProcessHidGamepadInputReportPtr CheckHidSwitchProGamepadSupport(uint32_t vendorId, uint32_t productId)
{
    if (vendorId == HID_SWITCH_VENDOR_ID && productId == HID_SWITCH_PRODUCT_ID)
    {
        return ProcessHidSwitchGamepadInputReport;
    }

    return nullptr;
}
