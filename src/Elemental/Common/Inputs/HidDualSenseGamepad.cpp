#pragma once

#include "../Elemental.h"
#include "Inputs.h"
#include "HidUtils.h"
#include "SystemFunctions.h"
#include "SystemDataPool.h"

// Based on the spec: https://controllers.fandom.com/wiki/Sony_DualSense

#define HID_DUALSENSE_VENDOR_ID 0x054C
#define HID_DUALSENSE_PRODUCT_ID 0x0CE6

#define HID_DUALSENSE_STICK_RANGE 255
#define HID_DUALSENSE_STICK_DEAD_ZONE 0.1f
#define HID_DUALSENSE_TOUCH_MAX_FINGERS 2
#define HID_DUALSENSE_TOUCH_DEAD_ZONE 0.75f
#define HID_DUALSENSE_TOUCH_SENSITIVITY 0.15f
#define HID_DUALSENSE_TOUCH_WIDTH 1920.0f
#define HID_DUALSENSE_TOUCH_HEIGHT 1070.0f
#define HID_DUALSENSE_ANGULAR_VELOCITY_DEAD_ZONE 0.1f
#define HID_DUALSENSE_ACCELEROMETER_DEAD_ZONE 0.05f

enum HidDualSenseGamepadInputReportType
{
    HidDualSenseGamepadInputReportType_BluetoothStandard = 0x01,
    HidDualSenseGamepadInputReportType_BluetoothExtended = 0x31,
};

enum HidDualSenseGamepadOutputReportType
{
    //HidDualSenseGamepadOutputReportType_SubCommand = 0x01
};

enum HidDualSenseGamepadFeatureReportType
{
    HidDualSenseGamepadFeatureReportType_Calibration = 0x05
};

enum HidDualSenseGamepadButton : uint16_t
{
    HidDualSenseGamepadButton_DpadUp = 1 << 0,
    HidDualSenseGamepadButton_DpadRight = 1 << 1,
    HidDualSenseGamepadButton_DpadDown = 1 << 2,
    HidDualSenseGamepadButton_DpadLeft = 1 << 3,
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

enum HidDualSenseGamepadSystemButton : uint8_t
{
    HidDualSenseGamepadSystemButton_Home = 1 << 0,
    HidDualSenseGamepadSystemButton_Pad = 1 << 1,
};

struct __attribute__((__packed__)) HidDualSenseGamepadTouchData
{
    uint32_t PackedData[2];
    uint8_t Timestamp;
};

struct __attribute__((__packed__)) HidDualSenseGamepadInputReport
{
    uint8_t LeftStickX;
    uint8_t LeftStickY;
    uint8_t RightStickX;
    uint8_t RightStickY;
    uint8_t LeftTrigger;
    uint8_t RightTrigger;
    uint8_t SequenceNumber;
    uint16_t Buttons;
    uint8_t SystemButtons;
    uint8_t Reserved1;
    uint32_t Reserved2;
    int16_t AngularVelocityX;
    int16_t AngularVelocityY;
    int16_t AngularVelocityZ;
    int16_t AccelerometerX;
    int16_t AccelerometerY;
    int16_t AccelerometerZ;
    uint32_t SensorTimestamp;
    int8_t Temperature;
    HidDualSenseGamepadTouchData TouchData;
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

struct __attribute__((__packed__)) HidDualSenseGamepadFeatureReportCalibration
{
    uint8_t ReportId;
    int16_t GyroPitchBias;
    int16_t GyroYawBias;
    int16_t GyroRollBias;
    int16_t GyroPitchPlus;
    int16_t GyroPitchMinus;
    int16_t GyroYawPlus;
    int16_t GyroYawMinus;
    int16_t GyroRollPlus;
    int16_t GyroRollMinus;
    int16_t GyroSpeedPlus;
    int16_t GyroSpeedMinus;
    int16_t AccelXPlus;
    int16_t AccelXMinus;
    int16_t AccelYPlus;
    int16_t AccelYMinus;
    int16_t AccelZPlus;
    int16_t AccelZMinus;
    int64_t Unknown;
};

struct HidDualSenseGamepadTouchState
{
    uint8_t FingerIndex; 
    bool IsTouching;
    int32_t PreviousPositionX;
    int32_t PreviousPositionY;
    float PreviousDeltaX;
    uint8_t PreviousDeltaXSteps;
    float PreviousDeltaY;
    uint8_t PreviousDeltaYSteps;
};

struct HidDualSenseGamepadData
{
    float PreviousLeftStickX;
    float PreviousLeftStickY;
    float PreviousRightStickX;
    float PreviousRightStickY;
    float PreviousLeftTrigger;
    float PreviousRightTrigger;
    uint16_t PreviousButtons;
    uint8_t PreviousMappedDpad;
    uint8_t PreviousSystemButtons;
    float PreviousAccelerometerX;
    float PreviousAccelerometerY;
    float PreviousAccelerometerZ;
    HidDualSenseGamepadTouchState TouchState[HID_DUALSENSE_TOUCH_MAX_FINGERS];
    bool IsCalibrated;
    HidDualSenseGamepadFeatureReportCalibration CalibrationData;
};

static SystemDataPool<HidDualSenseGamepadData, SystemDataPoolDefaultFull> hidDualSenseGamepadDataPool;

void InitHidDualSenseGamepadMemory()
{
    if (hidDualSenseGamepadDataPool.Storage == nullptr)
    {
        hidDualSenseGamepadDataPool = SystemCreateDataPool<HidDualSenseGamepadData>(InputsMemoryArena, MAX_INPUT_DEVICES);
    }
}

HidDualSenseGamepadData* GetHidDualSenseGamepadData(ElemHandle hidDevice)
{
    return SystemGetDataPoolItem(hidDualSenseGamepadDataPool, hidDevice);
}

uint8_t MapHidDualSenseGamepadDpad(uint8_t dpadData)
{
    static const uint8_t mapping[8] = {
        HidDualSenseGamepadButton_DpadUp,
        HidDualSenseGamepadButton_DpadUp | HidDualSenseGamepadButton_DpadRight,
        HidDualSenseGamepadButton_DpadRight,
        HidDualSenseGamepadButton_DpadRight | HidDualSenseGamepadButton_DpadDown,
        HidDualSenseGamepadButton_DpadDown,
        HidDualSenseGamepadButton_DpadDown | HidDualSenseGamepadButton_DpadLeft,
        HidDualSenseGamepadButton_DpadLeft,
        HidDualSenseGamepadButton_DpadLeft | HidDualSenseGamepadButton_DpadUp
    };

    return (dpadData & 8) ? 0 : mapping[dpadData % 8];
}

void ParseHidDualSenseGamepadTouchData(ElemWindow window, ElemInputDevice inputDevice, double elapsedSeconds, HidDualSenseGamepadTouchState* touchState, uint8_t fingerIndex, uint32_t touchData)
{
    auto isTouching = !((touchData >> 7) & 0x01);
    auto positionX = (int32_t)((touchData >> 8) & 0xFFF);
    auto positionY = (int32_t)((touchData >> 20) & 0xFFF);

    auto deltaX = 0.0f;
    auto deltaY = 0.0f;

    if (isTouching != touchState->IsTouching)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputDeviceTypeIndex = fingerIndex,
            .InputId = ElemInputId_Touch,
            .InputType = ElemInputType_Digital,
            .Value = isTouching ? 1.0f : 0.0f,
            .ElapsedSeconds = elapsedSeconds
        });

        if (!isTouching)
        {
            if (touchState->PreviousDeltaXSteps < 50)
            {
                deltaX = touchState->PreviousDeltaX;
            }

            if (touchState->PreviousDeltaYSteps < 50)
            {
                deltaY = touchState->PreviousDeltaY;
            }
        }
    }
    else 
    {
        deltaX = (float)(positionX - touchState->PreviousPositionX) * HID_DUALSENSE_TOUCH_SENSITIVITY;
        deltaY = (float)(positionY - touchState->PreviousPositionY) * (HID_DUALSENSE_TOUCH_WIDTH / HID_DUALSENSE_TOUCH_HEIGHT) * HID_DUALSENSE_TOUCH_SENSITIVITY;
    }

    if (deltaX < -HID_DUALSENSE_TOUCH_DEAD_ZONE)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputDeviceTypeIndex = fingerIndex,
            .InputId = ElemInputId_TouchXNegative,
            .InputType = ElemInputType_Delta,
            .Value = -deltaX,
            .ElapsedSeconds = elapsedSeconds
        });
    
        touchState->PreviousDeltaX = deltaX;
        touchState->PreviousDeltaXSteps = 0;
    }

    if (deltaX > HID_DUALSENSE_TOUCH_DEAD_ZONE)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputDeviceTypeIndex = fingerIndex,
            .InputId = ElemInputId_TouchXPositive,
            .InputType = ElemInputType_Delta,
            .Value = deltaX,
            .ElapsedSeconds = elapsedSeconds
        });
       
        touchState->PreviousDeltaX = deltaX;
        touchState->PreviousDeltaXSteps = 0;
    }

    if (deltaY < -HID_DUALSENSE_TOUCH_DEAD_ZONE)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputDeviceTypeIndex = fingerIndex,
            .InputId = ElemInputId_TouchYNegative,
            .InputType = ElemInputType_Delta,
            .Value = -deltaY,
            .ElapsedSeconds = elapsedSeconds
        });
    
        touchState->PreviousDeltaY = deltaY;
        touchState->PreviousDeltaYSteps = 0;
    }

    if (deltaY > HID_DUALSENSE_TOUCH_DEAD_ZONE)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputDeviceTypeIndex = fingerIndex,
            .InputId = ElemInputId_TouchYPositive,
            .InputType = ElemInputType_Delta,
            .Value = deltaY,
            .ElapsedSeconds = elapsedSeconds
        });
       
        touchState->PreviousDeltaY = deltaY;
        touchState->PreviousDeltaYSteps = 0;
    }

    if (touchState->PreviousDeltaXSteps == 0 || (isTouching != touchState->IsTouching))
    {
        auto normalizedPositionX = (float)positionX / HID_DUALSENSE_TOUCH_WIDTH;

        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputDeviceTypeIndex = fingerIndex,
            .InputId = ElemInputId_TouchXAbsolutePosition,
            .InputType = ElemInputType_Absolute,
            .Value = normalizedPositionX,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (touchState->PreviousDeltaYSteps == 0 || (isTouching != touchState->IsTouching))
    {
        auto normalizedPositionY = (float)positionY / HID_DUALSENSE_TOUCH_HEIGHT;

        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputDeviceTypeIndex = fingerIndex,
            .InputId = ElemInputId_TouchYAbsolutePosition,
            .InputType = ElemInputType_Absolute,
            .Value = -normalizedPositionY,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    touchState->IsTouching = isTouching;
    touchState->PreviousPositionX = positionX;
    touchState->PreviousPositionY = positionY;

    if (touchState->PreviousDeltaXSteps < 255)
    {
        touchState->PreviousDeltaXSteps += 1;
    }

    if (touchState->PreviousDeltaYSteps < 255)
    {
        touchState->PreviousDeltaYSteps += 1;
    }
}

float NormalizeHidDualSenseGamepadAngularVelocity(int16_t value, int16_t bias, int16_t thresholdMinus, int16_t thresholdPlus, int16_t speedMinus, int16_t speedPlus)
{
    float normalizedValue;

    if (value < 0)
    {
        normalizedValue = (float)(SystemMax(value, thresholdMinus) + bias) / speedMinus; 
    }
    else
    {
        normalizedValue = (float)(SystemMin(value, thresholdPlus) + bias) / speedPlus; 
    }

    if (normalizedValue < HID_DUALSENSE_ANGULAR_VELOCITY_DEAD_ZONE && normalizedValue > -HID_DUALSENSE_ANGULAR_VELOCITY_DEAD_ZONE)
    {
        return 0.0f;
    }

    return normalizedValue;
}

float NormalizeHidDualSenseGamepadAccelerometer(int16_t value, float previousValue, int16_t thresholdMinus, int16_t thresholdPlus)
{
    auto bias = (thresholdPlus + thresholdMinus) / 2.0f;
    auto scaleFactor = (thresholdPlus - thresholdMinus) / 2.0f;
    float normalizedValue = (value - bias) / scaleFactor;

    if (SystemAbs(normalizedValue - previousValue) < HID_DUALSENSE_ACCELEROMETER_DEAD_ZONE)
    {
        return previousValue;
    }

    return normalizedValue;
}

void ProcessHidDualSenseGamepadInputReport(ElemWindow window, ElemInputDevice inputDevice, HidDualSenseGamepadInputReport* inputReport, double elapsedSeconds)
{
    auto inputDeviceData = GetInputDeviceData(inputDevice);
    SystemAssert(inputDeviceData != ELEM_HANDLE_NULL);
    
    if (inputDeviceData->HidDeviceData != ELEM_HANDLE_NULL)
    {
        auto hidData = GetHidDualSenseGamepadData(inputDeviceData->HidDeviceData);
        SystemAssert(hidData);

        auto leftStickX = NormalizeInputValueSigned(inputReport->LeftStickX, HID_DUALSENSE_STICK_RANGE, HID_DUALSENSE_STICK_DEAD_ZONE);
        ProcessHidGamepadStick(window, inputDevice, elapsedSeconds, leftStickX, hidData->PreviousLeftStickX, ElemInputId_GamepadLeftStickXNegative, ElemInputId_GamepadLeftStickXPositive);

        auto leftStickY = -NormalizeInputValueSigned(inputReport->LeftStickY, HID_DUALSENSE_STICK_RANGE, HID_DUALSENSE_STICK_DEAD_ZONE);
        ProcessHidGamepadStick(window, inputDevice, elapsedSeconds, leftStickY, hidData->PreviousLeftStickY, ElemInputId_GamepadLeftStickYNegative, ElemInputId_GamepadLeftStickYPositive);

        auto rightStickX = NormalizeInputValueSigned(inputReport->RightStickX, HID_DUALSENSE_STICK_RANGE, HID_DUALSENSE_STICK_DEAD_ZONE);
        ProcessHidGamepadStick(window, inputDevice, elapsedSeconds, rightStickX, hidData->PreviousRightStickX, ElemInputId_GamepadRightStickXNegative, ElemInputId_GamepadRightStickXPositive);

        auto rightStickY = -NormalizeInputValueSigned(inputReport->RightStickY, HID_DUALSENSE_STICK_RANGE, HID_DUALSENSE_STICK_DEAD_ZONE);
        ProcessHidGamepadStick(window, inputDevice, elapsedSeconds, rightStickY, hidData->PreviousRightStickY, ElemInputId_GamepadRightStickYNegative, ElemInputId_GamepadRightStickYPositive);

        auto leftTrigger = NormalizeInputValue(inputReport->LeftTrigger, HID_DUALSENSE_STICK_RANGE, HID_DUALSENSE_STICK_DEAD_ZONE);
        ProcessHidGamepadTrigger(window, inputDevice, elapsedSeconds, leftTrigger, hidData->PreviousLeftTrigger, ElemInputId_GamepadLeftTrigger);

        auto rightTrigger = NormalizeInputValue(inputReport->RightTrigger, HID_DUALSENSE_STICK_RANGE, HID_DUALSENSE_STICK_DEAD_ZONE);
        ProcessHidGamepadTrigger(window, inputDevice, elapsedSeconds, rightTrigger, hidData->PreviousRightTrigger, ElemInputId_GamepadRightTrigger);

        auto mappedDpad = MapHidDualSenseGamepadDpad(inputReport->Buttons);

        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, mappedDpad, hidData->PreviousMappedDpad, HidDualSenseGamepadButton_DpadUp, ElemInputId_GamepadDpadUp);
        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, mappedDpad, hidData->PreviousMappedDpad, HidDualSenseGamepadButton_DpadRight, ElemInputId_GamepadDpadRight);
        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, mappedDpad, hidData->PreviousMappedDpad, HidDualSenseGamepadButton_DpadDown, ElemInputId_GamepadDpadDown);
        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, mappedDpad, hidData->PreviousMappedDpad, HidDualSenseGamepadButton_DpadLeft, ElemInputId_GamepadDpadLeft);

        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_Cross, ElemInputId_GamepadButtonA);
        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_Circle, ElemInputId_GamepadButtonB);
        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_Square, ElemInputId_GamepadButtonX);
        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_Triangle, ElemInputId_GamepadButtonY);
        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_L1, ElemInputId_GamepadLeftShoulder);
        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_R1, ElemInputId_GamepadRightShoulder);
        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_Share, ElemInputId_GamepadButtonShare);
        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_Options, ElemInputId_GamepadButtonOptions);
        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_L3, ElemInputId_GamepadLeftStickButton);
        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, inputReport->Buttons, hidData->PreviousButtons, HidDualSenseGamepadButton_R3, ElemInputId_GamepadRightStickButton);

        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, inputReport->SystemButtons, hidData->PreviousSystemButtons, HidDualSenseGamepadSystemButton_Home, ElemInputId_GamepadButtonHome);
        ProcessHidGamepadButton(window, inputDevice, elapsedSeconds, inputReport->SystemButtons, hidData->PreviousSystemButtons, HidDualSenseGamepadSystemButton_Pad, ElemInputId_GamepadTouchButton);

        // TODO: DualSense Edge buttons?

        if (hidData->IsCalibrated)
        {
            for (uint32_t i = 0; i < HID_DUALSENSE_TOUCH_MAX_FINGERS; i++)
            {
                ParseHidDualSenseGamepadTouchData(window, inputDevice, elapsedSeconds, &hidData->TouchState[i], i, inputReport->TouchData.PackedData[i]);
            }
        }

        auto calibration = hidData->CalibrationData;

        auto angularVelocityX = NormalizeHidDualSenseGamepadAngularVelocity(inputReport->AngularVelocityX, calibration.GyroPitchBias, calibration.GyroPitchMinus, calibration.GyroPitchPlus, calibration.GyroSpeedMinus, calibration.GyroSpeedPlus);
        ProcessHidGamepadDeltaAxe(window, inputDevice, elapsedSeconds, angularVelocityX, ElemInputId_AngularVelocityXNegative, ElemInputId_AngularVelocityXPositive);

        auto angularVelocityY = NormalizeHidDualSenseGamepadAngularVelocity(inputReport->AngularVelocityY, calibration.GyroYawBias, calibration.GyroYawMinus, calibration.GyroYawPlus, calibration.GyroSpeedMinus, calibration.GyroSpeedPlus);
        ProcessHidGamepadDeltaAxe(window, inputDevice, elapsedSeconds, angularVelocityY, ElemInputId_AngularVelocityYNegative, ElemInputId_AngularVelocityYPositive);

        auto angularVelocityZ = NormalizeHidDualSenseGamepadAngularVelocity(inputReport->AngularVelocityZ, calibration.GyroRollBias, calibration.GyroRollMinus, calibration.GyroRollPlus, calibration.GyroSpeedMinus, calibration.GyroSpeedPlus);
        ProcessHidGamepadDeltaAxe(window, inputDevice, elapsedSeconds, angularVelocityZ, ElemInputId_AngularVelocityZNegative, ElemInputId_AngularVelocityZPositive);

        auto accelerometerX = NormalizeHidDualSenseGamepadAccelerometer(inputReport->AccelerometerX, hidData->PreviousAccelerometerX, calibration.AccelXMinus, calibration.AccelXPlus);
        ProcessHidGamepadStick(window, inputDevice, elapsedSeconds, accelerometerX, hidData->PreviousAccelerometerX, ElemInputId_AccelerometerXNegative, ElemInputId_AccelerometerXPositive);

        auto accelerometerY = NormalizeHidDualSenseGamepadAccelerometer(inputReport->AccelerometerY, hidData->PreviousAccelerometerY, calibration.AccelYMinus, calibration.AccelYPlus);
        ProcessHidGamepadStick(window, inputDevice, elapsedSeconds, accelerometerY, hidData->PreviousAccelerometerY, ElemInputId_AccelerometerYNegative, ElemInputId_AccelerometerYPositive);

        auto accelerometerZ = NormalizeHidDualSenseGamepadAccelerometer(inputReport->AccelerometerZ, hidData->PreviousAccelerometerZ, calibration.AccelZMinus, calibration.AccelZPlus);
        ProcessHidGamepadStick(window, inputDevice, elapsedSeconds, accelerometerZ, hidData->PreviousAccelerometerZ, ElemInputId_AccelerometerZNegative, ElemInputId_AccelerometerZPositive);

        hidData->PreviousLeftStickX = leftStickX;
        hidData->PreviousLeftStickY = leftStickY;
        hidData->PreviousRightStickX = rightStickX;
        hidData->PreviousRightStickY = rightStickY;
        hidData->PreviousLeftTrigger = leftTrigger;
        hidData->PreviousRightTrigger = rightTrigger;
        hidData->PreviousButtons = inputReport->Buttons;
        hidData->PreviousMappedDpad = mappedDpad;
        hidData->PreviousSystemButtons = inputReport->SystemButtons;
        hidData->PreviousAccelerometerX = accelerometerX;
        hidData->PreviousAccelerometerY = accelerometerY;
        hidData->PreviousAccelerometerZ = accelerometerZ;
    }
}

void ProcessHidDualSenseGamepadInputReportBluetoothStandard(ElemWindow window, ElemInputDevice inputDevice, HidDualSenseGamepadInputReportBluetoothStandard* inputReport, double elapsedSeconds)
{
    SystemLogWarningMessage(ElemLogMessageCategory_Inputs, "Using DualSense GamePad Bluetooth Standard Report %f", elapsedSeconds);

    HidDualSenseGamepadInputReport mappedInputReport = 
    {
        .LeftStickX = inputReport->LeftStickX, 
        .LeftStickY = inputReport->LeftStickY,
        .RightStickX = inputReport->RightStickX,
        .RightStickY = inputReport->RightStickY,
        .LeftTrigger = inputReport->LeftTrigger,
        .RightTrigger = inputReport->RightTrigger,
        .Buttons = inputReport->Buttons,
        .SystemButtons = inputReport->SystemButtons
    };
    
    ProcessHidDualSenseGamepadInputReport(window, inputDevice, &mappedInputReport, elapsedSeconds);
}

void ProcessHidDualSenseGamepadInputReport(ElemWindow window, ElemInputDevice inputDevice, ReadOnlySpan<uint8_t> hidReport, double elapsedSeconds)
{
    // TODO: We need to differentiate USB / Bluetooth? How can we do that?

    InitHidDualSenseGamepadMemory();

    auto inputDeviceData = GetInputDeviceData(inputDevice);
    SystemAssert(inputDeviceData != ELEM_HANDLE_NULL);

    if (inputDeviceData->HidDeviceData == ELEM_HANDLE_NULL)
    {
        inputDeviceData->HidDeviceData = SystemAddDataPoolItem(hidDualSenseGamepadDataPool, {});
    }

    auto hidDeviceData = GetHidDualSenseGamepadData(inputDeviceData->HidDeviceData);
    SystemAssert(hidDeviceData);

    if (!hidDeviceData->IsCalibrated)
    {
        HidDualSenseGamepadFeatureReportCalibration calibrationFeatureReport =
        {
            .ReportId = HidDualSenseGamepadFeatureReportType_Calibration,
        };

        auto result = PlatformHidGetFeatureReport(inputDevice, ReadOnlySpan<uint8_t>((uint8_t*)&calibrationFeatureReport, sizeof(calibrationFeatureReport)));
        SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Reading DualSense Calibration Data: (Result=%d)", result);

        if (result)
        {
            hidDeviceData->IsCalibrated = true;
            hidDeviceData->CalibrationData = calibrationFeatureReport;

            // TODO: Set light of current player
            // TODO: Have a system to deactivate some features if not needed or to optin. (Like the gyro and accel)
        }
    }

    auto reportId = hidReport[0];
    
    if (reportId == HidDualSenseGamepadInputReportType_BluetoothStandard)
    {
        auto inputReport = (HidDualSenseGamepadInputReportBluetoothStandard*)(++hidReport.Pointer);
        ProcessHidDualSenseGamepadInputReportBluetoothStandard(window, inputDevice, inputReport, elapsedSeconds);
    }
    else if (reportId == HidDualSenseGamepadInputReportType_BluetoothExtended)
    {
        auto inputReport = (HidDualSenseGamepadInputReport*)(hidReport.Pointer + 2);
        ProcessHidDualSenseGamepadInputReport(window, inputDevice, inputReport, elapsedSeconds);
    }
}

ProcessHidGamepadInputReportPtr CheckHidDualSenseGamepadSupport(uint32_t vendorId, uint32_t productId)
{
    if (vendorId == HID_DUALSENSE_VENDOR_ID && productId == HID_DUALSENSE_PRODUCT_ID)
    {
        return ProcessHidDualSenseGamepadInputReport;
    }

    return nullptr;
}
