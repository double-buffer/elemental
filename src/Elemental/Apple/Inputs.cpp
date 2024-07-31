#include "Inputs.h"
#include "Inputs/Inputs.h"
#include "SystemDictionary.h"
#include "SystemLogging.h"
#include "SystemFunctions.h"
#include "SystemPlatformFunctions.h"

#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
#include "MacOSApplication.h"
#else
#include "UIKitApplication.h"
#endif

SystemDictionary<void*, ElemInputDevice> appleInputDeviceDictionary;

ElemInputDevice AddAppleInputDevice(void* device, ElemInputDeviceType deviceType)
{
    if (SystemDictionaryContainsKey(appleInputDeviceDictionary, device))
    {
        return *SystemGetDictionaryValue(appleInputDeviceDictionary, device);
    }

    auto stackMemoryArena = SystemGetStackMemoryArena();

    InputDeviceData deviceData =
    {
        .PlatformHandle = device,
        .InputDeviceType = deviceType
    };

    InputDeviceDataFull deviceDataFull = {};

    SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Create Input device.");
    auto handle = AddInputDevice(&deviceData, &deviceDataFull);
    SystemAddDictionaryEntry(appleInputDeviceDictionary, device, handle);

    return handle;
}

void RemoveAppleInputDevice(void* device)
{
    if (SystemDictionaryContainsKey(appleInputDeviceDictionary, device))
    {
        auto inputDevice = *SystemGetDictionaryValue(appleInputDeviceDictionary, device);
        RemoveInputDevice(inputDevice);
        SystemRemoveDictionaryEntry(appleInputDeviceDictionary, device);
    }
}

ElemInputId GetAppleInputIdFromKeyCode(GC::KeyCode keyCode)
{
    switch (keyCode)
    {
        case GC::KeyCode::GraveAccentAndTilde: return ElemInputId_KeyTilde;
        case GC::KeyCode::One: return ElemInputId_Key1;
        case GC::KeyCode::Two: return ElemInputId_Key2;
        case GC::KeyCode::Three: return ElemInputId_Key3;
        case GC::KeyCode::Four: return ElemInputId_Key4;
        case GC::KeyCode::Five: return ElemInputId_Key5;
        case GC::KeyCode::Six: return ElemInputId_Key6;
        case GC::KeyCode::Seven: return ElemInputId_Key7;
        case GC::KeyCode::Eight: return ElemInputId_Key8;
        case GC::KeyCode::Nine: return ElemInputId_Key9;
        case GC::KeyCode::Zero: return ElemInputId_Key0;
        case GC::KeyCode::Hyphen: return ElemInputId_KeyDash;
        case GC::KeyCode::EqualSign: return ElemInputId_KeyEquals;
        case GC::KeyCode::DeleteOrBackspace: return ElemInputId_KeyBackspace;
        case GC::KeyCode::Tab: return ElemInputId_KeyTab;
        case GC::KeyCode::KeyQ: return ElemInputId_KeyQ;
        case GC::KeyCode::KeyW: return ElemInputId_KeyW;
        case GC::KeyCode::KeyE: return ElemInputId_KeyE;
        case GC::KeyCode::KeyR: return ElemInputId_KeyR;
        case GC::KeyCode::KeyT: return ElemInputId_KeyT;
        case GC::KeyCode::KeyY: return ElemInputId_KeyY;
        case GC::KeyCode::KeyU: return ElemInputId_KeyU;
        case GC::KeyCode::KeyI: return ElemInputId_KeyI;
        case GC::KeyCode::KeyO: return ElemInputId_KeyO;
        case GC::KeyCode::KeyP: return ElemInputId_KeyP;
        case GC::KeyCode::OpenBracket: return ElemInputId_KeyLeftBrace;
        case GC::KeyCode::CloseBracket: return ElemInputId_KeyRightBrace;
        case GC::KeyCode::Backslash: return ElemInputId_KeyBackSlash;
        case GC::KeyCode::CapsLock: return ElemInputId_KeyCapsLock;
        case GC::KeyCode::KeyA: return ElemInputId_KeyA;
        case GC::KeyCode::KeyS: return ElemInputId_KeyS;
        case GC::KeyCode::KeyD: return ElemInputId_KeyD;
        case GC::KeyCode::KeyF: return ElemInputId_KeyF;
        case GC::KeyCode::KeyG: return ElemInputId_KeyG;
        case GC::KeyCode::KeyH: return ElemInputId_KeyH;
        case GC::KeyCode::KeyJ: return ElemInputId_KeyJ;
        case GC::KeyCode::KeyK: return ElemInputId_KeyK;
        case GC::KeyCode::KeyL: return ElemInputId_KeyL;
        case GC::KeyCode::Semicolon: return ElemInputId_KeySemiColon;
        case GC::KeyCode::Quote: return ElemInputId_KeyApostrophe;
        case GC::KeyCode::ReturnOrEnter: return ElemInputId_KeyEnter;
        case GC::KeyCode::LeftShift: return ElemInputId_KeyLeftShift;
        case GC::KeyCode::KeyZ: return ElemInputId_KeyZ;
        case GC::KeyCode::KeyX: return ElemInputId_KeyX;
        case GC::KeyCode::KeyC: return ElemInputId_KeyC;
        case GC::KeyCode::KeyV: return ElemInputId_KeyV;
        case GC::KeyCode::KeyB: return ElemInputId_KeyB;
        case GC::KeyCode::KeyN: return ElemInputId_KeyN;
        case GC::KeyCode::KeyM: return ElemInputId_KeyM;
        case GC::KeyCode::Comma: return ElemInputId_KeyComma;
        case GC::KeyCode::Period: return ElemInputId_KeyPeriod;
        case GC::KeyCode::Slash: return ElemInputId_KeySlash;
        case GC::KeyCode::RightShift: return ElemInputId_KeyRightShift;
        case GC::KeyCode::LeftControl: return ElemInputId_KeyLeftControl;
        case GC::KeyCode::LeftAlt: return ElemInputId_KeyLeftAlt;
        case GC::KeyCode::Spacebar: return ElemInputId_KeySpacebar;
        case GC::KeyCode::RightAlt: return ElemInputId_KeyRightAlt;
        case GC::KeyCode::RightControl: return ElemInputId_KeyRightControl;
        case GC::KeyCode::Insert: return ElemInputId_KeyInsert;
        case GC::KeyCode::DeleteForward: return ElemInputId_KeyDelete;
        case GC::KeyCode::LeftArrow: return ElemInputId_KeyLeftArrow;
        case GC::KeyCode::Home: return ElemInputId_KeyHome;
        case GC::KeyCode::End: return ElemInputId_KeyEnd;
        case GC::KeyCode::UpArrow: return ElemInputId_KeyUpArrow;
        case GC::KeyCode::DownArrow: return ElemInputId_KeyDownArrow;
        case GC::KeyCode::PageUp: return ElemInputId_KeyPageUp;
        case GC::KeyCode::PageDown: return ElemInputId_KeyPageDown;
        case GC::KeyCode::RightArrow: return ElemInputId_KeyRightArrow;
        case GC::KeyCode::KeypadNumLock: return ElemInputId_KeyNumpadLock;
        case GC::KeyCode::Keypad7: return ElemInputId_KeyNumpad7;
        case GC::KeyCode::Keypad4: return ElemInputId_KeyNumpad4;
        case GC::KeyCode::Keypad1: return ElemInputId_KeyNumpad1;
        case GC::KeyCode::KeypadSlash: return ElemInputId_KeyNumpadDivide;
        case GC::KeyCode::Keypad8: return ElemInputId_KeyNumpad8;
        case GC::KeyCode::Keypad5: return ElemInputId_KeyNumpad5;
        case GC::KeyCode::Keypad2: return ElemInputId_KeyNumpad2;
        case GC::KeyCode::Keypad0: return ElemInputId_KeyNumpad0;
        case GC::KeyCode::KeypadAsterisk: return ElemInputId_KeyNumpadMultiply;
        case GC::KeyCode::Keypad9: return ElemInputId_KeyNumpad9;
        case GC::KeyCode::Keypad6: return ElemInputId_KeyNumpad6;
        case GC::KeyCode::Keypad3: return ElemInputId_KeyNumpad3;
        case GC::KeyCode::KeypadPeriod: return ElemInputId_KeyNumpadSeparator;
        case GC::KeyCode::KeypadHyphen: return ElemInputId_KeyNumpadMinus;
        case GC::KeyCode::KeypadPlus: return ElemInputId_KeyNumpadAdd;
        case GC::KeyCode::KeypadEnter: return ElemInputId_KeyNumpadEnter;
        case GC::KeyCode::Escape: return ElemInputId_KeyEscape;
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
        case GC::KeyCode::PrintScreen: return ElemInputId_KeyPrintScreen;
        case GC::KeyCode::ScrollLock: return ElemInputId_KeyScrollLock;
        case GC::KeyCode::Pause: return ElemInputId_KeyPause;
        case GC::KeyCode::LeftGui: return ElemInputId_KeyLeftSystem;
        case GC::KeyCode::RightGui: return ElemInputId_KeyRightSystem;
        case GC::KeyCode::Application: return ElemInputId_KeyApp;
        default: return ElemInputId_Unknown;
    }
}

void KeyboardHandler(ElemWindow window, GC::KeyboardInput* keyboardInput, GC::ControllerButtonInput* controllerButton, GC::KeyCode keyCode, bool isPressed)
{
    auto elapsedSeconds = (double)(SystemPlatformGetHighPerformanceCounter() - ApplePerformanceCounterStart) / ApplePerformanceCounterFrequencyInSeconds;
    auto inputDevice = AddAppleInputDevice(keyboardInput->device(), ElemInputDeviceType_Keyboard);

    // TODO: We need to find a way to handle key repeats!

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = GetAppleInputIdFromKeyCode(keyCode),
        .InputType = ElemInputType_Digital,
        .Value = isPressed ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });
}

void MouseMoveHandler(ElemWindow window, GC::MouseInput* mouse, float deltaX, float deltaY)
{
    auto elapsedSeconds = (double)(SystemPlatformGetHighPerformanceCounter() - ApplePerformanceCounterStart) / ApplePerformanceCounterFrequencyInSeconds;
    auto inputDevice = AddAppleInputDevice(mouse->device(), ElemInputDeviceType_Mouse);

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
            .Value = (float)deltaY,
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
            .Value = -(float)deltaY,
            .ElapsedSeconds = elapsedSeconds
        });
    }
}
    
void ButtonHandler(ElemWindow window, ElemInputId inputId, GC::Device* device, GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
{
    auto elapsedSeconds = (double)(SystemPlatformGetHighPerformanceCounter() - ApplePerformanceCounterStart) / ApplePerformanceCounterFrequencyInSeconds;
    auto inputDevice = AddAppleInputDevice(device, ElemInputDeviceType_Mouse);

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputId = inputId,
        .InputType = ElemInputType_Digital,
        .Value = value,
        .ElapsedSeconds = elapsedSeconds
    });
}

// TODO: Keep track of old value because otherwise we send multiple 0

void DirectionHandler(ElemWindow window, AppleGamepadDirection gamepadDirection, GC::Device* device, GC::ControllerDirectionPad* directionPad, float xValue, float yValue)
{
    auto elapsedSeconds = (double)(SystemPlatformGetHighPerformanceCounter() - ApplePerformanceCounterStart) / ApplePerformanceCounterFrequencyInSeconds;
    auto inputDevice = AddAppleInputDevice(device, ElemInputDeviceType_Mouse);

    if (gamepadDirection == AppleGamepadDirection::LeftStick)
    {
        if (xValue >= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_GamepadLeftStickXPositive,
                .InputType = ElemInputType_Analog,
                .Value = xValue,
                .ElapsedSeconds = elapsedSeconds
            });
        }
        
        if (xValue <= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_GamepadLeftStickXNegative,
                .InputType = ElemInputType_Analog,
                .Value = -xValue,
                .ElapsedSeconds = elapsedSeconds
            });
        }
        
        if (yValue >= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_GamepadLeftStickYPositive,
                .InputType = ElemInputType_Analog,
                .Value = yValue,
                .ElapsedSeconds = elapsedSeconds
            });
        }
        
        if (yValue <= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_GamepadLeftStickYNegative,
                .InputType = ElemInputType_Analog,
                .Value = -yValue,
                .ElapsedSeconds = elapsedSeconds
            });
        }
    }
    else if (gamepadDirection == AppleGamepadDirection::RightStick)
    {
        if (xValue >= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_GamepadRightStickXPositive,
                .InputType = ElemInputType_Analog,
                .Value = xValue,
                .ElapsedSeconds = elapsedSeconds
            });
        }
        
        if (xValue <= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_GamepadRightStickXNegative,
                .InputType = ElemInputType_Analog,
                .Value = -xValue,
                .ElapsedSeconds = elapsedSeconds
            });
        }
        
        if (yValue >= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_GamepadRightStickYPositive,
                .InputType = ElemInputType_Analog,
                .Value = yValue,
                .ElapsedSeconds = elapsedSeconds
            });
        }
        
        if (yValue <= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_GamepadRightStickYNegative,
                .InputType = ElemInputType_Analog,
                .Value = -yValue,
                .ElapsedSeconds = elapsedSeconds
            });
        }
    }
    else if (gamepadDirection == AppleGamepadDirection::Dpad)
    {
        if (xValue >= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_GamepadDpadRight,
                .InputType = ElemInputType_Digital,
                .Value = xValue,
                .ElapsedSeconds = elapsedSeconds
            });
        }
        
        if (xValue <= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_GamepadDpadLeft,
                .InputType = ElemInputType_Digital,
                .Value = -xValue,
                .ElapsedSeconds = elapsedSeconds
            });
        }
        
        if (yValue >= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_GamepadDpadUp,
                .InputType = ElemInputType_Digital,
                .Value = yValue,
                .ElapsedSeconds = elapsedSeconds
            });
        }
        
        if (yValue <= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_GamepadDpadDown,
                .InputType = ElemInputType_Digital,
                .Value = -yValue,
                .ElapsedSeconds = elapsedSeconds
            });
        }
    }
    else if (gamepadDirection == AppleGamepadDirection::MouseWheel)
    {
        const float multiplier = 3.0f;

        if (xValue >= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_MouseHorizontalWheelPositive,
                .InputType = ElemInputType_Delta,
                .Value = xValue * multiplier,
                .ElapsedSeconds = elapsedSeconds
            });
        }
        
        if (xValue <= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_MouseHorizontalWheelNegative,
                .InputType = ElemInputType_Delta,
                .Value = -xValue * multiplier,
                .ElapsedSeconds = elapsedSeconds
            });
        }
        
        if (yValue >= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_MouseWheelPositive,
                .InputType = ElemInputType_Delta,
                .Value = yValue * multiplier,
                .ElapsedSeconds = elapsedSeconds
            });
        }
        
        if (yValue <= 0.0f)
        {
            AddInputEvent({
                .Window = window,
                .InputDevice = inputDevice,
                .InputId = ElemInputId_MouseWheelNegative,
                .InputType = ElemInputType_Delta,
                .Value = -yValue * multiplier,
                .ElapsedSeconds = elapsedSeconds
            });
        }
    }
}

void TouchHandler(ElemWindow window, void* deviceId, uint32_t fingerIndex, float x, float y, float deltaX, float deltaY, uint32_t state)
{
    auto elapsedSeconds = (double)(SystemPlatformGetHighPerformanceCounter() - ApplePerformanceCounterStart) / ApplePerformanceCounterFrequencyInSeconds;
    auto inputDevice = AddAppleInputDevice(deviceId, ElemInputDeviceType_Touch);

    // TODO: Do we need to remove the touch device?
    //SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Test touch: deltaX=%f, deltaY=%f, state=%d (device: %d, finger index: %d, x=%f, y=%f)", deltaX, deltaY, state, deviceId, fingerIndex, x, y);

    if (state == 0 || state == 2)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputDeviceTypeIndex = fingerIndex,
            .InputId = ElemInputId_Touch,
            .InputType = ElemInputType_Digital,
            .Value = state == 0 ? 1.0f : 0.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (deltaX < 0)
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
    }

    if (deltaX > 0)
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
    }

    if (deltaY < 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputDeviceTypeIndex = fingerIndex,
            .InputId = ElemInputId_TouchYNegative,
            .InputType = ElemInputType_Delta,
            .Value = deltaY,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    if (deltaY > 0)
    {
        AddInputEvent({
            .Window = window,
            .InputDevice = inputDevice,
            .InputDeviceTypeIndex = fingerIndex,
            .InputId = ElemInputId_TouchYPositive,
            .InputType = ElemInputType_Delta,
            .Value = -deltaY,
            .ElapsedSeconds = elapsedSeconds
        });
    }

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputDeviceTypeIndex = fingerIndex,
        .InputId = ElemInputId_TouchXAbsolutePosition,
        .InputType = ElemInputType_Absolute,
        .Value = x,
        .ElapsedSeconds = elapsedSeconds
    });

    AddInputEvent({
        .Window = window,
        .InputDevice = inputDevice,
        .InputDeviceTypeIndex = fingerIndex,
        .InputId = ElemInputId_TouchYAbsolutePosition,
        .InputType = ElemInputType_Absolute,
        .Value = y,
        .ElapsedSeconds = elapsedSeconds
    });
}

// TODO: We need to lock the cursor in fullscreen for ipad and iphone
// See: https://developer.apple.com/wwdc20/10617

void InitInputs(ElemWindow window)
{
    appleInputDeviceDictionary = SystemCreateDictionary<void*, ElemInputDevice>(ApplicationMemoryArena, MAX_INPUT_DEVICES);

    NS::NotificationCenter::defaultCenter()->addObserver(MTLSTR("GCKeyboardDidConnectNotification"), nullptr, nullptr, ^(NS::Notification* notification)
    {
        auto keyboard = (GC::Keyboard*)notification->object();
        keyboard->keyboardInput()->setKeyChangedHandler(^(GC::KeyboardInput* keyboardInput, GC::ControllerButtonInput* controllerButton, GC::KeyCode keyCode, bool isPressed)
        {
            KeyboardHandler(window, keyboardInput, controllerButton, keyCode, isPressed);
        });
    });

    NS::NotificationCenter::defaultCenter()->addObserver(MTLSTR("GCKeyboardDidDisconnectNotification"), nullptr, nullptr, ^(NS::Notification* notification)
    {
        auto keyboard = (GC::Keyboard*)notification->object();
        RemoveAppleInputDevice(keyboard->keyboardInput()->device());
    });

    NS::NotificationCenter::defaultCenter()->addObserver(MTLSTR("GCMouseDidConnectNotification"), nullptr, nullptr, ^(NS::Notification* notification)
    {
        auto mouse = (GC::Mouse*)notification->object();
        auto mouseInput = mouse->mouseInput();

        mouseInput->setMouseMovedHandler(^(GC::MouseInput* mouse, float deltaX, float deltaY)
        {
            MouseMoveHandler(window, mouse, deltaX, deltaY);
        });

        mouseInput->leftButton()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            auto cursorPosition = ElemGetWindowCursorPosition(window);

            if ((cursorPosition.X > 0 && cursorPosition.Y > 0) || !isPressed)
            {
                ButtonHandler(window, ElemInputId_MouseLeftButton, mouseInput->device(), controllerButtonInput, value, isPressed);
            }
        });
        
        mouseInput->middleButton()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            auto cursorPosition = ElemGetWindowCursorPosition(window);

            if ((cursorPosition.X > 0 && cursorPosition.Y > 0) || !isPressed)
            {
                ButtonHandler(window, ElemInputId_MouseMiddleButton, mouseInput->device(), controllerButtonInput, value, isPressed);
            }
        });

        mouseInput->rightButton()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            auto cursorPosition = ElemGetWindowCursorPosition(window);

            if ((cursorPosition.X > 0 && cursorPosition.Y > 0) || !isPressed)
            {
                ButtonHandler(window, ElemInputId_MouseRightButton, mouseInput->device(), controllerButtonInput, value, isPressed);
            }
        });

        mouseInput->scroll()->setValueChangedHandler(^(GC::ControllerDirectionPad* directionPad, float xValue, float yValue)
        {
            DirectionHandler(window, AppleGamepadDirection::MouseWheel, mouseInput->device(), directionPad, xValue, yValue);
        });

        auto mouseExtraButtons = mouseInput->auxiliaryButtons();

        if (mouseExtraButtons)
        {
            for (uint32_t i = 0; i < mouseExtraButtons->count() && i < 2; i++)
            {
                ((GC::ControllerButtonInput*)mouseExtraButtons->object(i))->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
                {
                    auto cursorPosition = ElemGetWindowCursorPosition(window);

                    if ((cursorPosition.X > 0 && cursorPosition.Y > 0) || !isPressed)
                    {
                        // TODO: Check the with other platforms if the extra buttons are in the same order
                        ButtonHandler(window, i == 0 ? ElemInputId_MouseExtraButton1 : ElemInputId_MouseExtraButton2, mouseInput->device(), controllerButtonInput, value, isPressed);
                    }
                });
            }
        }
    });

    NS::NotificationCenter::defaultCenter()->addObserver(MTLSTR("GCMouseDidDisconnectNotification"), nullptr, nullptr, ^(NS::Notification* notification)
    {
        auto mouse = (GC::Mouse*)notification->object();
        RemoveAppleInputDevice(mouse->mouseInput()->device());
    });

    NS::NotificationCenter::defaultCenter()->addObserver(MTLSTR("GCControllerDidConnectNotification"), nullptr, nullptr, ^(NS::Notification* notification)
    {
        auto controller = (GC::Controller*)notification->object();
        auto extendedGamepad = controller->extendedGamepad();

        if (!extendedGamepad)
        {
            SystemLogWarningMessage(ElemLogMessageCategory_Inputs, "Gamepad not supported.");
            return;
        }

        extendedGamepad->buttonA()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            ButtonHandler(window, ElemInputID_GamepadButtonA, extendedGamepad->device(), controllerButtonInput, value, isPressed);
        });

        extendedGamepad->buttonB()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            ButtonHandler(window, ElemInputID_GamepadButtonB, extendedGamepad->device(), controllerButtonInput, value, isPressed);
        });

        extendedGamepad->buttonX()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            ButtonHandler(window, ElemInputID_GamepadButtonX, extendedGamepad->device(), controllerButtonInput, value, isPressed);
        });

        extendedGamepad->buttonY()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            ButtonHandler(window, ElemInputID_GamepadButtonY, extendedGamepad->device(), controllerButtonInput, value, isPressed);
        });

        extendedGamepad->buttonMenu()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            ButtonHandler(window, ElemInputID_GamepadButtonMenu, extendedGamepad->device(), controllerButtonInput, value, isPressed);
        });

        extendedGamepad->buttonOptions()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            ButtonHandler(window, ElemInputID_GamepadButtonOptions, extendedGamepad->device(), controllerButtonInput, value, isPressed);
        });

        extendedGamepad->buttonHome()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            ButtonHandler(window, ElemInputID_GamepadButtonHome, extendedGamepad->device(), controllerButtonInput, value, isPressed);
        });

        extendedGamepad->leftShoulder()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            ButtonHandler(window, ElemInputID_GamepadLeftShoulder, extendedGamepad->device(), controllerButtonInput, value, isPressed);
        });

        extendedGamepad->rightShoulder()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            ButtonHandler(window, ElemInputID_GamepadRightShoulder, extendedGamepad->device(), controllerButtonInput, value, isPressed);
        });

        extendedGamepad->leftTrigger()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            ButtonHandler(window, ElemInputID_GamepadLeftTrigger, extendedGamepad->device(), controllerButtonInput, value, isPressed);
        });

        extendedGamepad->rightTrigger()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            ButtonHandler(window, ElemInputID_GamepadRightTrigger, extendedGamepad->device(), controllerButtonInput, value, isPressed);
        });

        extendedGamepad->leftThumbstick()->setValueChangedHandler(^(GC::ControllerDirectionPad* directionPad, float xValue, float yValue)
        {
            DirectionHandler(window, AppleGamepadDirection::LeftStick, extendedGamepad->device(), directionPad, xValue, yValue);
        });

        extendedGamepad->leftThumbstickButton()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            ButtonHandler(window, ElemInputId_GamepadLeftStickButton, extendedGamepad->device(), controllerButtonInput, value, isPressed);
        });

        extendedGamepad->rightThumbstick()->setValueChangedHandler(^(GC::ControllerDirectionPad* directionPad, float xValue, float yValue)
        {
            DirectionHandler(window, AppleGamepadDirection::RightStick, extendedGamepad->device(), directionPad, xValue, yValue);
        });

        extendedGamepad->rightThumbstickButton()->setValueChangedHandler(^(GC::ControllerButtonInput* controllerButtonInput, float value, bool isPressed)
        {
            ButtonHandler(window, ElemInputId_GamepadRightStickButton, extendedGamepad->device(), controllerButtonInput, value, isPressed);
        });

        extendedGamepad->dpad()->setValueChangedHandler(^(GC::ControllerDirectionPad* directionPad, float xValue, float yValue)
        {
            DirectionHandler(window, AppleGamepadDirection::Dpad, extendedGamepad->device(), directionPad, xValue, yValue);
        });
    });

    NS::NotificationCenter::defaultCenter()->addObserver(MTLSTR("GCControllerDidDisconnectNotification"), nullptr, nullptr, ^(NS::Notification* notification)
    {
        auto controller = (GC::Controller*)notification->object();
        RemoveAppleInputDevice(controller->physicalInputProfile()->device());
    });
}
