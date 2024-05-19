#include "Inputs.h"
#include "Inputs/Inputs.h"
#include "SystemDictionary.h"
#include "SystemLogging.h"
#include "SystemFunctions.h"

#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
#include "MacOSApplication.h"
#else
#include "UIKitApplication.h"
#endif

SystemDictionary<GC::Device*, ElemInputDevice> appleInputDeviceDictionary;

ElemInputDevice AddAppleInputDevice(GC::Device* device, ElemInputDeviceType deviceType)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    InputDeviceData deviceData =
    {
        .PlatformHandle = device,
        .InputDeviceType = deviceType
    };

    InputDeviceDataFull deviceDataFull = {};
/*
    if (device->productCategory() == RIM_TYPEMOUSE)
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
    }*/

    SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Create Input device.");
    auto handle = AddInputDevice(&deviceData, &deviceDataFull);
    SystemAddDictionaryEntry(appleInputDeviceDictionary, device, handle);

    return handle;
}

ElemInputId GetAppleInputIdFromKeyCode(GC::KeyCode keyCode)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "KeyCode: %d", keyCode);
    switch (keyCode)
    {
        case GC::KeyCode::Zero: return ElemInputId_Key0;
        case GC::KeyCode::One: return ElemInputId_Key1;
        case GC::KeyCode::Two: return ElemInputId_Key2;
        case GC::KeyCode::Three: return ElemInputId_Key3;
        case GC::KeyCode::Four: return ElemInputId_Key4;
        case GC::KeyCode::Five: return ElemInputId_Key5;
        case GC::KeyCode::Six: return ElemInputId_Key6;
        case GC::KeyCode::Seven: return ElemInputId_Key7;
        case GC::KeyCode::Eight: return ElemInputId_Key8;
        case GC::KeyCode::Nine: return ElemInputId_Key9;
        case GC::KeyCode::KeyA: return ElemInputId_KeyA;
        case GC::KeyCode::KeyB: return ElemInputId_KeyB;
        case GC::KeyCode::KeyC: return ElemInputId_KeyC;
        case GC::KeyCode::KeyD: return ElemInputId_KeyD;
        case GC::KeyCode::KeyE: return ElemInputId_KeyE;
        case GC::KeyCode::KeyF: return ElemInputId_KeyF;
        case GC::KeyCode::KeyG: return ElemInputId_KeyG;
        case GC::KeyCode::KeyH: return ElemInputId_KeyH;
        case GC::KeyCode::KeyI: return ElemInputId_KeyI;
        case GC::KeyCode::KeyJ: return ElemInputId_KeyJ;
        case GC::KeyCode::KeyK: return ElemInputId_KeyK;
        case GC::KeyCode::KeyL: return ElemInputId_KeyL;
        case GC::KeyCode::KeyM: return ElemInputId_KeyM;
        case GC::KeyCode::KeyN: return ElemInputId_KeyN;
        case GC::KeyCode::KeyO: return ElemInputId_KeyO;
        case GC::KeyCode::KeyP: return ElemInputId_KeyP;
        case GC::KeyCode::KeyQ: return ElemInputId_KeyQ;
        case GC::KeyCode::KeyR: return ElemInputId_KeyR;
        case GC::KeyCode::KeyS: return ElemInputId_KeyS;
        case GC::KeyCode::KeyT: return ElemInputId_KeyT;
        case GC::KeyCode::KeyU: return ElemInputId_KeyU;
        case GC::KeyCode::KeyV: return ElemInputId_KeyV;
        case GC::KeyCode::KeyW: return ElemInputId_KeyW;
        case GC::KeyCode::KeyX: return ElemInputId_KeyX;
        case GC::KeyCode::KeyY: return ElemInputId_KeyY;
        case GC::KeyCode::KeyZ: return ElemInputId_KeyZ;
        case GC::KeyCode::DeleteOrBackspace: return ElemInputId_KeyBack;
        case GC::KeyCode::Tab: return ElemInputId_KeyTab;
        //case GC::KeyCode::DeleteForward: return ElemInputId_KeyClear;
        case GC::KeyCode::ReturnOrEnter: return ElemInputId_KeyReturn;
        //case GC::KeyCode::LeftControl: return ElemInputId_KeyControl;
        //case GC::KeyCode::Tacb: return ElemInputId_KeyMenu;
        case GC::KeyCode::Pause: return ElemInputId_KeyPause;
        case GC::KeyCode::CapsLock: return ElemInputId_KeyCapsLock;
        case GC::KeyCode::Escape: return ElemInputId_KeyEscape;
        case GC::KeyCode::Spacebar: return ElemInputId_KeySpace;
        case GC::KeyCode::PageUp: return ElemInputId_KeyPageUp;
        case GC::KeyCode::PageDown: return ElemInputId_KeyPageDown;
        case GC::KeyCode::End: return ElemInputId_KeyEnd;
        case GC::KeyCode::Home: return ElemInputId_KeyHome;
        case GC::KeyCode::LeftArrow: return ElemInputId_KeyLeft;
        case GC::KeyCode::UpArrow: return ElemInputId_KeyUp;
        case GC::KeyCode::RightArrow: return ElemInputId_KeyRight;
        case GC::KeyCode::DownArrow: return ElemInputId_KeyDown;
        case GC::KeyCode::KeypadEnter: return ElemInputId_KeyExecute;
        case GC::KeyCode::PrintScreen: return ElemInputId_KeyPrintScreen;
        case GC::KeyCode::Insert: return ElemInputId_KeyInsert;
        case GC::KeyCode::DeleteForward: return ElemInputId_KeyDelete;
        //case GC::KeyCode::LeftGui: return ElemInputId_KeyLeftSystemButton;
        //case GC::KeyCode::RightGui: return ElemInputId_KeyRightSystemButton;
        case GC::KeyCode::Keypad0: return ElemInputId_KeyNumpad0;
        case GC::KeyCode::Keypad1: return ElemInputId_KeyNumpad1;
        case GC::KeyCode::Keypad2: return ElemInputId_KeyNumpad2;
        case GC::KeyCode::Keypad3: return ElemInputId_KeyNumpad3;
        case GC::KeyCode::Keypad4: return ElemInputId_KeyNumpad4;
        case GC::KeyCode::Keypad5: return ElemInputId_KeyNumpad5;
        case GC::KeyCode::Keypad6: return ElemInputId_KeyNumpad6;
        case GC::KeyCode::Keypad7: return ElemInputId_KeyNumpad7;
        case GC::KeyCode::Keypad8: return ElemInputId_KeyNumpad8;
        case GC::KeyCode::Keypad9: return ElemInputId_KeyNumpad9;
        case GC::KeyCode::KeypadAsterisk: return ElemInputId_KeyMultiply;
        case GC::KeyCode::KeypadPlus: return ElemInputId_KeyAdd;
        //case GC::KeyCode::KeypadHyphen: return ElemInputId_KeySeparator;
        //case GC::KeyCode::KeypadHyphen: return ElemInputId_KeySubtract;
        case GC::KeyCode::KeypadPeriod: return ElemInputId_KeyDecimal;
        case GC::KeyCode::KeypadSlash: return ElemInputId_KeyDivide;
        case GC::KeyCode::F1: return ElemInputId_KeyF1;
        case GC::KeyCode::F2: return ElemInputId_KeyF2;
        case GC::KeyCode::F3: return ElemInputId_KeyF3;
        case GC::KeyCode::F4: return ElemInputId_KeyF4;
        case GC::KeyCode::F5: return ElemInputId_KeyF5;
        case GC::KeyCode::F6: return ElemInputId_KeyF6;
        case GC::KeyCode::F7: return ElemInputId_KeyF7;
        case GC::KeyCode::F8: return ElemInputId_KeyF8;
        case GC::KeyCode::F9: return ElemInputId_KeyF9;
        case GC::KeyCode::F10: return ElemInputId_KeyF10;
        case GC::KeyCode::F11: return ElemInputId_KeyF11;
        case GC::KeyCode::F12: return ElemInputId_KeyF12;
        case GC::KeyCode::LeftShift: return ElemInputId_KeyLeftShift;
        case GC::KeyCode::RightShift: return ElemInputId_KeyRightShift;
        case GC::KeyCode::LeftControl: return ElemInputId_KeyLeftControl;
        case GC::KeyCode::RightControl: return ElemInputId_KeyRightControl;
        case GC::KeyCode::LeftAlt: return ElemInputId_KeyLeftAlt;
        case GC::KeyCode::RightAlt: return ElemInputId_KeyRightAlt;
        default: return ElemInputId_Unknown;
    }
}

void KeyboardHandler(GC::KeyboardInput* keyboardInput, GC::ControllerButtonInput* controllerButton, GC::KeyCode keyCode, bool isPressed)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Handler OK! %d", isPressed);

    //auto stackMemoryArena = SystemGetStackMemoryArena();

    //auto elapsedSeconds = (double)(SystemPlatformGetHighPerformanceCounter() - Win32PerformanceCounterStart) / Win32PerformanceCounterFrequencyInSeconds;

    // TODO: Extract that to a separate function
    ElemInputDevice inputDevice;

    if (!SystemDictionaryContainsKey(appleInputDeviceDictionary, keyboardInput->device()))
    {
        inputDevice = AddAppleInputDevice(keyboardInput->device(), ElemInputDeviceType_Keyboard);
    }
    else
    {
        inputDevice = *SystemGetDictionaryValue(appleInputDeviceDictionary, keyboardInput->device());
    }

    auto inputDeviceData = GetInputDeviceData(inputDevice);
    SystemAssert(inputDeviceData);

    // TODO: We need to find a way to handle key repeats!

    AddInputEvent({
        .Window = 1,//window, // TODO: 
        .InputDevice = inputDevice,
        .InputId = GetAppleInputIdFromKeyCode(keyCode),
        .InputType = ElemInputType_Digital,
        .Value = isPressed ? 1.0f : 0.0f,
        .ElapsedSeconds = 0.0//elapsedSeconds // TODO:
    });
}

// TODO: We need to lock the cursort in fullscreen for ipad and iphone
// See: https://developer.apple.com/wwdc20/10617

// TODO: Compute correct timestamp like in Win32
// TODO: Deactivate beep sound

void InitInputs(ElemWindow window)
{
    appleInputDeviceDictionary = SystemCreateDictionary<GC::Device*, ElemInputDevice>(ApplicationMemoryArena, MAX_INPUT_DEVICES);

    NS::NotificationCenter::defaultCenter()->addObserver(MTLSTR("GCKeyboardDidConnectNotification"), nullptr, nullptr, ^(NS::Notification* notification)
    {
        auto keyboard = (GC::Keyboard*)notification->object();
        SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Test %s", keyboard->keyboardInput()->device()->productCategory()->utf8String());

        keyboard->keyboardInput()->setKeyChangedHandler(KeyboardHandler);
    });
}
