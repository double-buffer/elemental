#include "Win32Inputs.h"
#include "Win32Application.h"
#include "Inputs/Inputs.h"
#include "Inputs/HidDevices.h"
#include "SystemDictionary.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemPlatformFunctions.h"

SystemDictionary<HANDLE, ElemInputDevice> win32InputDeviceDictionary;

ElemInputDevice AddWin32RawInputDevice(HANDLE device, DWORD type)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    uint32_t rawInputDeviceInfoSize;
    AssertIfFailed(GetRawInputDeviceInfo(device, RIDI_DEVICEINFO, nullptr, &rawInputDeviceInfoSize));

    auto rawInputDeviceInfo = (RID_DEVICE_INFO*)SystemPushArray<uint8_t>(stackMemoryArena, rawInputDeviceInfoSize).Pointer;
    rawInputDeviceInfo->cbSize = sizeof(RID_DEVICE_INFO);

    AssertIfFailed(GetRawInputDeviceInfo(device, RIDI_DEVICEINFO, rawInputDeviceInfo, &rawInputDeviceInfoSize)); 

    InputDeviceData deviceData =
    {
        .PlatformHandle = device,
        .InputDeviceType = ElemInputDeviceType_Unknown
    };

    InputDeviceDataFull deviceDataFull = {};

    if (type == RIM_TYPEMOUSE)
    {
        deviceData.InputDeviceType = ElemInputDeviceType_Mouse;
    }
    else if (type == RIM_TYPEKEYBOARD)
    {
        deviceData.InputDeviceType = ElemInputDeviceType_Keyboard;
    }
    else if (type == RIM_TYPEHID)
    {
        if (!IsHidDeviceSupported(rawInputDeviceInfo->hid.dwVendorId, rawInputDeviceInfo->hid.dwProductId))
        {
            return ELEM_HANDLE_NULL;
        }
        
        deviceData.InputDeviceType = ElemInputDeviceType_Gamepad;
        deviceData.HidVendorId = rawInputDeviceInfo->hid.dwVendorId;
        deviceData.HidProductId = rawInputDeviceInfo->hid.dwProductId;
    }

    SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Create Input device.");
    auto handle = AddInputDevice(&deviceData, &deviceDataFull);
    SystemAddDictionaryEntry(win32InputDeviceDictionary, device, handle);

    return handle;
}

void InitWin32Inputs(HWND window)
{
    RAWINPUTDEVICE devices[] =
    {
        {
            .usUsagePage = HID_USAGE_PAGE_GENERIC,
            .usUsage = HID_USAGE_GENERIC_MOUSE,
            .dwFlags = RIDEV_DEVNOTIFY,
            .hwndTarget = window
        },
        {
            .usUsagePage = HID_USAGE_PAGE_GENERIC,
            .usUsage = HID_USAGE_GENERIC_KEYBOARD,
            .dwFlags = RIDEV_DEVNOTIFY,
            .hwndTarget = window
        },
        {
            .usUsagePage = HID_USAGE_PAGE_GENERIC,
            .usUsage = HID_USAGE_GENERIC_GAMEPAD,
            .dwFlags = RIDEV_DEVNOTIFY,
            .hwndTarget = window
        }
    };

    if (!RegisterRawInputDevices(devices, ARRAYSIZE(devices), sizeof(RAWINPUTDEVICE))) 
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Inputs, "Cannot init RawInput.", ARRAYSIZE(devices));
    }

    win32InputDeviceDictionary = SystemCreateDictionary<HANDLE, ElemInputDevice>(ApplicationMemoryArena, MAX_INPUT_DEVICES);

    auto stackMemoryArena = SystemGetStackMemoryArena();

    uint32_t devicesListLength;
    AssertIfFailed(GetRawInputDeviceList(nullptr, &devicesListLength, sizeof(RAWINPUTDEVICELIST)));

    auto devicesList = SystemPushArray<RAWINPUTDEVICELIST>(stackMemoryArena, devicesListLength);
    AssertIfFailed(GetRawInputDeviceList(devicesList.Pointer, &devicesListLength, sizeof(RAWINPUTDEVICELIST)));
    
    for (uint32_t i = 0; i < devicesListLength; i++)
    {
        auto device = devicesList[i];

        if (device.dwType == RIM_TYPEMOUSE || device.dwType == RIM_TYPEKEYBOARD || device.dwType == RIM_TYPEHID)
        {
            if (!SystemDictionaryContainsKey(win32InputDeviceDictionary, device.hDevice))
            {
                AddWin32RawInputDevice(device.hDevice, device.dwType);
            }
        }
    }
}

ElemInputId GetWin32InputIdFromMakeCode(WPARAM wParam)
{
    switch (wParam)
    {
        case Win32Scancode_KeyTilde: return ElemInputId_KeyTilde;
        case Win32Scancode_Key1: return ElemInputId_Key1;
        case Win32Scancode_Key2: return ElemInputId_Key2;
        case Win32Scancode_Key3: return ElemInputId_Key3;
        case Win32Scancode_Key4: return ElemInputId_Key4;
        case Win32Scancode_Key5: return ElemInputId_Key5;
        case Win32Scancode_Key6: return ElemInputId_Key6;
        case Win32Scancode_Key7: return ElemInputId_Key7;
        case Win32Scancode_Key8: return ElemInputId_Key8;
        case Win32Scancode_Key9: return ElemInputId_Key9;
        case Win32Scancode_Key0: return ElemInputId_Key0;
        case Win32Scancode_KeyDash: return ElemInputId_KeyDash;
        case Win32Scancode_KeyEquals: return ElemInputId_KeyEquals;
        case Win32Scancode_KeyBackspace: return ElemInputId_KeyBackspace;
        case Win32Scancode_KeyTab: return ElemInputId_KeyTab;
        case Win32Scancode_KeyQ: return ElemInputId_KeyQ;
        case Win32Scancode_KeyW: return ElemInputId_KeyW;
        case Win32Scancode_KeyE: return ElemInputId_KeyE;
        case Win32Scancode_KeyR: return ElemInputId_KeyR;
        case Win32Scancode_KeyT: return ElemInputId_KeyT;
        case Win32Scancode_KeyY: return ElemInputId_KeyY;
        case Win32Scancode_KeyU: return ElemInputId_KeyU;
        case Win32Scancode_KeyI: return ElemInputId_KeyI;
        case Win32Scancode_KeyO: return ElemInputId_KeyO;
        case Win32Scancode_KeyP: return ElemInputId_KeyP;
        case Win32Scancode_KeyLeftBrace: return ElemInputId_KeyLeftBrace;
        case Win32Scancode_KeyRightBrace: return ElemInputId_KeyRightBrace;
        case Win32Scancode_KeyBackSlash: return ElemInputId_KeyBackSlash;
        case Win32Scancode_KeyCapsLock: return ElemInputId_KeyCapsLock;
        case Win32Scancode_KeyA: return ElemInputId_KeyA;
        case Win32Scancode_KeyS: return ElemInputId_KeyS;
        case Win32Scancode_KeyD: return ElemInputId_KeyD;
        case Win32Scancode_KeyF: return ElemInputId_KeyF;
        case Win32Scancode_KeyG: return ElemInputId_KeyG;
        case Win32Scancode_KeyH: return ElemInputId_KeyH;
        case Win32Scancode_KeyJ: return ElemInputId_KeyJ;
        case Win32Scancode_KeyK: return ElemInputId_KeyK;
        case Win32Scancode_KeyL: return ElemInputId_KeyL;
        case Win32Scancode_KeySemiColon: return ElemInputId_KeySemiColon;
        case Win32Scancode_KeyApostrophe: return ElemInputId_KeyApostrophe;
        case Win32Scancode_KeyEnter: return ElemInputId_KeyEnter;
        case Win32Scancode_KeyLeftShift: return ElemInputId_KeyLeftShift;
        case Win32Scancode_KeyZ: return ElemInputId_KeyZ;
        case Win32Scancode_KeyX: return ElemInputId_KeyX;
        case Win32Scancode_KeyC: return ElemInputId_KeyC;
        case Win32Scancode_KeyV: return ElemInputId_KeyV;
        case Win32Scancode_KeyB: return ElemInputId_KeyB;
        case Win32Scancode_KeyN: return ElemInputId_KeyN;
        case Win32Scancode_KeyM: return ElemInputId_KeyM;
        case Win32Scancode_KeyComma: return ElemInputId_KeyComma;
        case Win32Scancode_KeyPeriod: return ElemInputId_KeyPeriod;
        case Win32Scancode_KeySlash: return ElemInputId_KeySlash;
        case Win32Scancode_KeyRightShift: return ElemInputId_KeyRightShift;
        case Win32Scancode_KeyLeftControl: return ElemInputId_KeyLeftControl;
        case Win32Scancode_KeyLeftAlt: return ElemInputId_KeyLeftAlt;
        case Win32Scancode_KeySpacebar: return ElemInputId_KeySpacebar;
        case Win32Scancode_KeyRightAlt: return ElemInputId_KeyRightAlt;
        case Win32Scancode_KeyRightControl: return ElemInputId_KeyRightControl;
        case Win32Scancode_KeyInsert: return ElemInputId_KeyInsert;
        case Win32Scancode_KeyDelete: return ElemInputId_KeyDelete;
        case Win32Scancode_KeyLeftArrow: return ElemInputId_KeyLeftArrow;
        case Win32Scancode_KeyHome: return ElemInputId_KeyHome;
        case Win32Scancode_KeyEnd: return ElemInputId_KeyEnd;
        case Win32Scancode_KeyUpArrow: return ElemInputId_KeyUpArrow;
        case Win32Scancode_KeyDownArrow: return ElemInputId_KeyDownArrow;
        case Win32Scancode_KeyPageUp: return ElemInputId_KeyPageUp;
        case Win32Scancode_KeyPageDown: return ElemInputId_KeyPageDown;
        case Win32Scancode_KeyRightArrow: return ElemInputId_KeyRightArrow;
        case Win32Scancode_KeyNumpadLock: return ElemInputId_KeyNumpadLock;
        case Win32Scancode_KeyNumpad7: return ElemInputId_KeyNumpad7;
        case Win32Scancode_KeyNumpad4: return ElemInputId_KeyNumpad4;
        case Win32Scancode_KeyNumpad1: return ElemInputId_KeyNumpad1;
        case Win32Scancode_KeyNumpadDivide: return ElemInputId_KeyNumpadDivide;
        case Win32Scancode_KeyNumpad8: return ElemInputId_KeyNumpad8;
        case Win32Scancode_KeyNumpad5: return ElemInputId_KeyNumpad5;
        case Win32Scancode_KeyNumpad2: return ElemInputId_KeyNumpad2;
        case Win32Scancode_KeyNumpad0: return ElemInputId_KeyNumpad0;
        case Win32Scancode_KeyNumpadMultiply: return ElemInputId_KeyNumpadMultiply;
        case Win32Scancode_KeyNumpad9: return ElemInputId_KeyNumpad9;
        case Win32Scancode_KeyNumpad6: return ElemInputId_KeyNumpad6;
        case Win32Scancode_KeyNumpad3: return ElemInputId_KeyNumpad3;
        case Win32Scancode_KeyNumpadSeparator: return ElemInputId_KeyNumpadSeparator;
        case Win32Scancode_KeyNumpadMinus: return ElemInputId_KeyNumpadMinus;
        case Win32Scancode_KeyNumpadAdd: return ElemInputId_KeyNumpadAdd;
        case Win32Scancode_KeyNumpadEnter: return ElemInputId_KeyNumpadEnter;
        case Win32Scancode_KeyEscape: return ElemInputId_KeyEscape;
        case Win32Scancode_KeyF1: return ElemInputId_KeyF1;
        case Win32Scancode_KeyF2: return ElemInputId_KeyF2;
        case Win32Scancode_KeyF3: return ElemInputId_KeyF3;
        case Win32Scancode_KeyF4: return ElemInputId_KeyF4;
        case Win32Scancode_KeyF5: return ElemInputId_KeyF5;
        case Win32Scancode_KeyF6: return ElemInputId_KeyF6;
        case Win32Scancode_KeyF7: return ElemInputId_KeyF7;
        case Win32Scancode_KeyF8: return ElemInputId_KeyF8;
        case Win32Scancode_KeyF9: return ElemInputId_KeyF9;
        case Win32Scancode_KeyF10: return ElemInputId_KeyF10;
        case Win32Scancode_KeyF11: return ElemInputId_KeyF11;
        case Win32Scancode_KeyF12: return ElemInputId_KeyF12;
        case Win32Scancode_KeyPrintScreen: return ElemInputId_KeyPrintScreen;
        case Win32Scancode_KeyScrollLock: return ElemInputId_KeyScrollLock;
        case Win32Scancode_KeyPause: return ElemInputId_KeyPause;
        case Win32Scancode_KeyLeftSystem: return ElemInputId_KeyLeftSystem;
        case Win32Scancode_KeyRightSystem: return ElemInputId_KeyRightSystem;
        case Win32Scancode_KeyApp: return ElemInputId_KeyApp;
        default: return ElemInputId_Unknown;
    };
}

void ProcessWin32RawInputKeyboard(ElemWindow window, ElemInputDevice inputDevice, RAWKEYBOARD* keyboardData, double elapsedSeconds)
{
    USHORT makeCode = keyboardData->MakeCode;

    if (keyboardData->Flags & RI_KEY_E0)
    {
        makeCode |= 0xE000; // Prefix with 0xE0 for extended keys
    }

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = GetWin32InputIdFromMakeCode(makeCode),
        .InputType = ElemInputType_Digital,
        .Value = keyboardData->Message == WM_KEYDOWN ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });
}

void ProcessWin32RawInputMouse(ElemWindow window, ElemInputDevice inputDevice, RAWMOUSE* mouseData, double elapsedSeconds)
{
    if (mouseData->usFlags != MOUSE_MOVE_RELATIVE)
    {
        SystemLogWarningMessage(ElemLogMessageCategory_Inputs, "Mouse position other than relative is not supported for the moment.");
        return;
    }

    int deltaX = mouseData->lLastX;
    int deltaY = mouseData->lLastY;

    if (deltaX < 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseAxisXNegative,
            .InputType = ElemInputType_Delta,
            .Value = -(float)deltaX,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (deltaX > 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseAxisXPositive,
            .InputType = ElemInputType_Delta,
            .Value = (float)deltaX,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (deltaY < 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseAxisYNegative,
            .InputType = ElemInputType_Delta,
            .Value = -(float)deltaY,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (deltaY > 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseAxisYPositive,
            .InputType = ElemInputType_Delta,
            .Value = (float)deltaY,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (mouseData->ulButtons & RI_MOUSE_LEFT_BUTTON_DOWN)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseLeftButton,
            .InputType = ElemInputType_Digital,
            .Value = 1.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (mouseData->ulButtons & RI_MOUSE_LEFT_BUTTON_UP)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseLeftButton,
            .InputType = ElemInputType_Digital,
            .Value = 0.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (mouseData->ulButtons & RI_MOUSE_RIGHT_BUTTON_DOWN)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseRightButton,
            .InputType = ElemInputType_Digital,
            .Value = 1.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (mouseData->ulButtons & RI_MOUSE_RIGHT_BUTTON_UP)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseRightButton,
            .InputType = ElemInputType_Digital,
            .Value = 0.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (mouseData->ulButtons & RI_MOUSE_MIDDLE_BUTTON_DOWN)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseMiddleButton,
            .InputType = ElemInputType_Digital,
            .Value = 1.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (mouseData->ulButtons & RI_MOUSE_MIDDLE_BUTTON_UP)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseMiddleButton,
            .InputType = ElemInputType_Digital,
            .Value = 0.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (mouseData->ulButtons & RI_MOUSE_BUTTON_4_DOWN)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseExtraButton1,
            .InputType = ElemInputType_Digital,
            .Value = 1.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (mouseData->ulButtons & RI_MOUSE_BUTTON_4_UP)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseExtraButton1,
            .InputType = ElemInputType_Digital,
            .Value = 0.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (mouseData->ulButtons & RI_MOUSE_BUTTON_5_DOWN)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseExtraButton2,
            .InputType = ElemInputType_Digital,
            .Value = 1.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (mouseData->ulButtons & RI_MOUSE_BUTTON_5_UP)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseExtraButton2,
            .InputType = ElemInputType_Digital,
            .Value = 0.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (mouseData->ulButtons & RI_MOUSE_WHEEL)
    {
        auto wheelDelta = (float)(short)mouseData->usButtonData;

        if (wheelDelta < 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_MouseWheelNegative,
                .InputType = ElemInputType_Delta,
                .Value = -wheelDelta / WHEEL_DELTA,
                .ElapsedSeconds = elapsedSeconds
            });
        }

        if (wheelDelta > 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_MouseWheelPositive,
                .InputType = ElemInputType_Delta,
                .Value = wheelDelta / WHEEL_DELTA,
                .ElapsedSeconds = elapsedSeconds
            });
        }
    }

    if (mouseData->ulButtons & RI_MOUSE_HWHEEL)
    {
        auto wheelDelta = (float)(short)mouseData->usButtonData;

        if (wheelDelta < 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_MouseHorizontalWheelNegative,
                .InputType = ElemInputType_Delta,
                .Value = -wheelDelta / WHEEL_DELTA,
                .ElapsedSeconds = elapsedSeconds
            });
        }

        if (wheelDelta > 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_MouseHorizontalWheelPositive,
                .InputType = ElemInputType_Delta,
                .Value = wheelDelta / WHEEL_DELTA,
                .ElapsedSeconds = elapsedSeconds
            });
        }
    }
}

void RemoveWin32InputDevice(HANDLE rawInputDevice)
{
    if (SystemDictionaryContainsKey(win32InputDeviceDictionary, rawInputDevice))
    {
        auto inputDevice = *SystemGetDictionaryValue(win32InputDeviceDictionary, rawInputDevice);
        RemoveInputDevice(inputDevice);
    }
}

void ProcessWin32RawInput(ElemWindow window, LPARAM lParam)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto elapsedSeconds = (double)(SystemPlatformGetHighPerformanceCounter() - Win32PerformanceCounterStart) / Win32PerformanceCounterFrequencyInSeconds;

    uint32_t rawInputDataSize;
    AssertIfFailed(GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &rawInputDataSize, sizeof(RAWINPUTHEADER)));

    auto rawInputData = (RAWINPUT*)SystemPushArray<uint8_t>(stackMemoryArena, rawInputDataSize).Pointer;
    AssertIfFailed(GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawInputData, &rawInputDataSize, sizeof(RAWINPUTHEADER)));

    if (!rawInputData->header.hDevice)
    {
        return;
    }

    ElemInputDevice inputDevice;

    if (!SystemDictionaryContainsKey(win32InputDeviceDictionary, rawInputData->header.hDevice))
    {
        inputDevice = AddWin32RawInputDevice(rawInputData->header.hDevice, rawInputData->header.dwType);
    }
    else
    {
        inputDevice = *SystemGetDictionaryValue(win32InputDeviceDictionary, rawInputData->header.hDevice);
    }

    if (inputDevice == ELEM_HANDLE_NULL)
    {
        SystemLogWarningMessage(ElemLogMessageCategory_Inputs, "Input device is not supported.");
        return;
    }

    auto inputDeviceData = GetInputDeviceData(inputDevice);
    SystemAssert(inputDeviceData);

    if (inputDeviceData->InputDeviceType == ElemInputDeviceType_Keyboard)
    {
        ProcessWin32RawInputKeyboard(window, inputDevice, &rawInputData->data.keyboard, elapsedSeconds);
    }
    else if (inputDeviceData->InputDeviceType == ElemInputDeviceType_Mouse) 
    {
        ProcessWin32RawInputMouse(window, inputDevice, &rawInputData->data.mouse, elapsedSeconds);
    }
    else if (inputDeviceData->InputDeviceType == ElemInputDeviceType_Gamepad)
    {
        ProcessHidDeviceData(window, inputDevice, ReadOnlySpan<uint8_t>(rawInputData->data.hid.bRawData, rawInputData->data.hid.dwSizeHid), elapsedSeconds);
    }
}
