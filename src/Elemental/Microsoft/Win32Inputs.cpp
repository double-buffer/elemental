#include "Win32Inputs.h"
#include "Win32Application.h"
#include "Inputs/Inputs.h"
#include "SystemDictionary.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemPlatformFunctions.h"

SystemDictionary<HANDLE, ElemInputDevice> win32InputDeviceDictionary;

void InitWin32Inputs(HWND window)
{
    // TODO: To Review

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
}

ElemInputId GetWin32InputIdFromKeyCode(WPARAM wParam)
{
    switch (wParam)
    {
        case '0': return ElemInputId_Key0;
        case '1': return ElemInputId_Key1;
        case '2': return ElemInputId_Key2;
        case '3': return ElemInputId_Key3;
        case '4': return ElemInputId_Key4;
        case '5': return ElemInputId_Key5;
        case '6': return ElemInputId_Key6;
        case '7': return ElemInputId_Key7;
        case '8': return ElemInputId_Key8;
        case '9': return ElemInputId_Key9;
        case 'A': return ElemInputId_KeyA;
        case 'B': return ElemInputId_KeyB;
        case 'C': return ElemInputId_KeyC;
        case 'D': return ElemInputId_KeyD;
        case 'E': return ElemInputId_KeyE;
        case 'F': return ElemInputId_KeyF;
        case 'G': return ElemInputId_KeyG;
        case 'H': return ElemInputId_KeyH;
        case 'I': return ElemInputId_KeyI;
        case 'J': return ElemInputId_KeyJ;
        case 'K': return ElemInputId_KeyK;
        case 'L': return ElemInputId_KeyL;
        case 'M': return ElemInputId_KeyM;
        case 'N': return ElemInputId_KeyN;
        case 'O': return ElemInputId_KeyO;
        case 'P': return ElemInputId_KeyP;
        case 'Q': return ElemInputId_KeyQ;
        case 'R': return ElemInputId_KeyR;
        case 'S': return ElemInputId_KeyS;
        case 'T': return ElemInputId_KeyT;
        case 'U': return ElemInputId_KeyU;
        case 'V': return ElemInputId_KeyV;
        case 'W': return ElemInputId_KeyW;
        case 'X': return ElemInputId_KeyX;
        case 'Y': return ElemInputId_KeyY;
        case 'Z': return ElemInputId_KeyZ;
        case VK_BACK: return ElemInputId_KeyBack;
        case VK_TAB: return ElemInputId_KeyTab;
        case VK_CLEAR: return ElemInputId_KeyClear;
        case VK_RETURN: return ElemInputId_KeyReturn;
        case VK_CONTROL: return ElemInputId_KeyControl;
        case VK_APPS: return ElemInputId_KeyMenu;
        case VK_PAUSE: return ElemInputId_KeyPause;
        case VK_CAPITAL: return ElemInputId_KeyCapsLock;
        case VK_ESCAPE: return ElemInputId_KeyEscape;
        case VK_SPACE: return ElemInputId_KeySpace;
        case VK_PRIOR: return ElemInputId_KeyPageUp;
        case VK_NEXT: return ElemInputId_KeyPageDown;
        case VK_END: return ElemInputId_KeyEnd;
        case VK_HOME: return ElemInputId_KeyHome;
        case VK_LEFT: return ElemInputId_KeyLeft;
        case VK_UP: return ElemInputId_KeyUp;
        case VK_RIGHT: return ElemInputId_KeyRight;
        case VK_DOWN: return ElemInputId_KeyDown;
        case VK_EXECUTE: return ElemInputId_KeyExecute;
        case VK_SNAPSHOT: return ElemInputId_KeyPrintScreen;
        case VK_INSERT: return ElemInputId_KeyInsert;
        case VK_DELETE: return ElemInputId_KeyDelete;
        case VK_LWIN: return ElemInputId_KeyLeftSystemButton;
        case VK_RWIN: return ElemInputId_KeyRightSystemButton;
        case VK_NUMPAD0: return ElemInputId_KeyNumpad0;
        case VK_NUMPAD1: return ElemInputId_KeyNumpad1;
        case VK_NUMPAD2: return ElemInputId_KeyNumpad2;
        case VK_NUMPAD3: return ElemInputId_KeyNumpad3;
        case VK_NUMPAD4: return ElemInputId_KeyNumpad4;
        case VK_NUMPAD5: return ElemInputId_KeyNumpad5;
        case VK_NUMPAD6: return ElemInputId_KeyNumpad6;
        case VK_NUMPAD7: return ElemInputId_KeyNumpad7;
        case VK_NUMPAD8: return ElemInputId_KeyNumpad8;
        case VK_NUMPAD9: return ElemInputId_KeyNumpad9;
        case VK_MULTIPLY: return ElemInputId_KeyMultiply;
        case VK_ADD: return ElemInputId_KeyAdd;
        case VK_SEPARATOR: return ElemInputId_KeySeparator;
        case VK_SUBTRACT: return ElemInputId_KeySubtract;
        case VK_DECIMAL: return ElemInputId_KeyDecimal;
        case VK_DIVIDE: return ElemInputId_KeyDivide;
        case VK_F1: return ElemInputId_KeyF1;
        case VK_F2: return ElemInputId_KeyF2;
        case VK_F3: return ElemInputId_KeyF3;
        case VK_F4: return ElemInputId_KeyF4;
        case VK_F5: return ElemInputId_KeyF5;
        case VK_F6: return ElemInputId_KeyF6;
        case VK_F7: return ElemInputId_KeyF7;
        case VK_F8: return ElemInputId_KeyF8;
        case VK_F9: return ElemInputId_KeyF9;
        case VK_F10: return ElemInputId_KeyF10;
        case VK_F11: return ElemInputId_KeyF11;
        case VK_F12: return ElemInputId_KeyF12;
        case VK_LSHIFT: return ElemInputId_KeyLeftShift;
        case VK_RSHIFT: return ElemInputId_KeyRightShift;
        case VK_LCONTROL: return ElemInputId_KeyLeftControl;
        case VK_RCONTROL: return ElemInputId_KeyRightControl;
        case VK_LMENU: return ElemInputId_KeyLeftAlt;
        case VK_RMENU: return ElemInputId_KeyRightAlt;
        default: return ElemInputId_Unknown;
    };
}

void ProcessWin32RawInputKeyboard(ElemWindow window, ElemInputDevice inputDevice, RAWKEYBOARD* keyboardData, double elapsedSeconds)
{
    ElemInputEvent event =
    {
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = GetWin32InputIdFromKeyCode(keyboardData->VKey),
        .InputType = ElemInputType_Digital,
        .Value = keyboardData->Message == WM_KEYDOWN ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    };

    AddInputEvent(event);
}

void ProcessWin32RawInputMouse(ElemWindow window, ElemInputDevice inputDevice, RAWMOUSE* mouseData, double elapsedSeconds)
{
    // TODO: Handle absolute coordinates if the flag is set!
    // TODO: For the absolute coordinates if we have only delta. Maybe we can get the current cursor postion on the window and create other events
    // for the absolute coordinates. This way we can keep the InputEvent struct light

    int deltaX = mouseData->lLastX;
    int deltaY = mouseData->lLastY;

    if (deltaX != 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseAxisX,
            .InputType = ElemInputType_Delta,
            .Value = (float)deltaX,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (deltaY != 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseAxisY,
            .InputType = ElemInputType_Delta,
            .Value = (float)deltaY,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (mouseData->ulButtons & RI_MOUSE_BUTTON_1_DOWN)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseLeft,
            .InputType = ElemInputType_Digital,
            .Value = 1.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (mouseData->ulButtons & RI_MOUSE_BUTTON_1_UP)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputId = ElemInputId_MouseLeft,
            .InputType = ElemInputType_Digital,
            .Value = 0.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }
}

typedef enum : uint32_t 
{
    HidGamepadVendor_Microsoft = 0x045E,
    HidGamepadVendor_Sony = 0x054C
} HidGamepadVendor;

typedef enum : uint32_t 
{
    HidGamepadProduct_XboxOneWirelessOldDriver = 0x02E0,
    HidGamepadProduct_XboxOneUsb = 0x02FF,
    HidGamepadProduct_XboxOneWireless = 0x02FD,
    HidGamepadProduct_DualShock4OldDriver = 0x5C4,
    HidGamepadProduct_DualShock4 = 0x9cc
} HidGamepadProduct;

typedef PackedStruct
{
    uint8_t Padding;
    uint16_t LeftStickX;
    uint16_t LeftStickY;
    uint16_t RightStickX;
    uint16_t RightStickY;
    uint16_t Triggers;
    uint16_t Buttons;
    uint8_t Dpad;
} XboxOneWirelessOldDriverGamepadReport;

// TODO: Handle device removal
ElemInputDevice AddWin32RawInputDevice(RAWINPUT* rawInputData)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Creating Input device");

        UINT bufferSize = 0;
        GetRawInputDeviceInfo(rawInputData->header.hDevice, RIDI_DEVICEINFO, nullptr, &bufferSize);

        auto rawInputDeviceInfo = (RID_DEVICE_INFO*)SystemPushArray<uint8_t>(stackMemoryArena, bufferSize).Pointer;
    rawInputDeviceInfo->cbSize = sizeof(RID_DEVICE_INFO);
    if (GetRawInputDeviceInfo(rawInputData->header.hDevice, RIDI_DEVICEINFO, rawInputDeviceInfo, &bufferSize) == -1) {
            SystemLogErrorMessage(ElemLogMessageCategory_Inputs, "Error");
    }
    // TODO: Fill additional data based on the device type
    InputDeviceData deviceData =
    {
        .PlatformHandle = rawInputData->header.hDevice,
        .InputDeviceType = ElemInputDeviceType_Unknown
    };

    InputDeviceDataFull deviceDataFull =
    {
    };

    // TODO: Fill additional infos
    if (rawInputData->header.dwType == RIM_TYPEMOUSE)
    {
        deviceData.InputDeviceType = ElemInputDeviceType_Mouse;
    }
    else if (rawInputData->header.dwType == RIM_TYPEKEYBOARD)
    {
        deviceData.InputDeviceType = ElemInputDeviceType_Keyboard;
    }
    else if (rawInputData->header.dwType == RIM_TYPEHID)
    {
        // TODO: Extract the check in a function
        if (!(rawInputDeviceInfo->hid.dwVendorId == HidGamepadVendor_Microsoft && rawInputDeviceInfo->hid.dwProductId == HidGamepadProduct_XboxOneUsb))
        {
            SystemLogWarningMessage(ElemLogMessageCategory_Inputs, "Unknown hid device");
            return ELEM_HANDLE_NULL;
        }
        
        deviceData.InputDeviceType = ElemInputDeviceType_Gamepad;
        deviceData.HidVendorId = rawInputDeviceInfo->hid.dwVendorId;
        deviceData.HidProductId = rawInputDeviceInfo->hid.dwProductId;
    }

    auto handle = AddInputDevice(&deviceData, &deviceDataFull);
    SystemAddDictionaryEntry(win32InputDeviceDictionary, rawInputData->header.hDevice, handle);

    return handle;
}

void ProcessWin32RawInput(ElemWindow window, LPARAM lParam)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto elapsedSeconds = (double)(SystemPlatformGetHighPerformanceCounter() - Win32PerformanceCounterStart) / Win32PerformanceCounterFrequencyInSeconds;

    UINT rawInputDataSize;
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
        inputDevice = AddWin32RawInputDevice(rawInputData);
    }
    else
    {
        inputDevice = *SystemGetDictionaryValue(win32InputDeviceDictionary, rawInputData->header.hDevice);
    }

    auto inputDeviceData = GetInputDeviceData(inputDevice);
    SystemAssert(inputDeviceData);

    // TODO: Handle other mouse buttons and maybe the whole keyboard too

    // TODO: Split into multiple functions
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
        // TODO: Extract that to a common function
        if (inputDeviceData->HidVendorId == HidGamepadVendor_Microsoft && inputDeviceData->HidProductId == HidGamepadProduct_XboxOneUsb)
        {
            auto xboxReport = (XboxOneWirelessOldDriverGamepadReport*)rawInputData->data.hid.bRawData;
            SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "RawInput HID: %d", xboxReport->Dpad);
        }
    }
}
