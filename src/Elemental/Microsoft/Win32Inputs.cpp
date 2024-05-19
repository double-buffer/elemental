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
        deviceDataFull.MouseNumberOfButtons = rawInputDeviceInfo->mouse.dwNumberOfButtons;
        deviceDataFull.MouseSampleRate = rawInputDeviceInfo->mouse.dwSampleRate;
    }
    else if (type == RIM_TYPEKEYBOARD)
    {
        deviceData.InputDeviceType = ElemInputDeviceType_Keyboard;
        deviceDataFull.KeyboardNumberOfKeys = rawInputDeviceInfo->keyboard.dwNumberOfKeysTotal;

        switch (rawInputDeviceInfo->keyboard.dwType)
        {
            case 0x7:
                deviceDataFull.KeyboardType = ElemKeyboardType_Japanese;
                break;

            case 0x8:
                deviceDataFull.KeyboardType = ElemKeyboardType_Korean;
                break;

            default:
                deviceDataFull.KeyboardType = ElemKeyboardType_Normal;
        }
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
        deviceDataFull.GamepadVersion = rawInputDeviceInfo->hid.dwVersionNumber;
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
    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = GetWin32InputIdFromKeyCode(keyboardData->VKey),
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
