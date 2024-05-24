#include "WaylandInputs.h"
#include "WaylandApplication.h"
#include "WaylandWindow.h"
#include "Inputs/Inputs.h"
#include "SystemDictionary.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemPlatformFunctions.h"

SystemDictionary<void*, ElemInputDevice> waylandInputDeviceDictionary;

wl_seat* waylandSeat = nullptr;
wl_keyboard* waylandKeyboard = nullptr;
wl_pointer* WaylandPointer = nullptr;
zwp_relative_pointer_manager_v1* waylandRelativePointerManager = nullptr;
zwp_relative_pointer_v1* waylandRelativePointer = nullptr;

ElemInputDevice AddWaylandInputDevice(void* device, ElemInputDeviceType deviceType)
{
    if (SystemDictionaryContainsKey(waylandInputDeviceDictionary, device))
    {
        return *SystemGetDictionaryValue(waylandInputDeviceDictionary, device);
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
    SystemAddDictionaryEntry(waylandInputDeviceDictionary, device, handle);

    return handle;
}

ElemInputId GetWaylandInputIdFromKeyCode(uint32_t keyCode)
{
    switch (keyCode)
    {
        case KEY_GRAVE: return ElemInputId_KeyTilde;
        case KEY_1: return ElemInputId_Key1;
        case KEY_2: return ElemInputId_Key2;
        case KEY_3: return ElemInputId_Key3;
        case KEY_4: return ElemInputId_Key4;
        case KEY_5: return ElemInputId_Key5;
        case KEY_6: return ElemInputId_Key6;
        case KEY_7: return ElemInputId_Key7;
        case KEY_8: return ElemInputId_Key8;
        case KEY_9: return ElemInputId_Key9;
        case KEY_0: return ElemInputId_Key0;
        case KEY_MINUS: return ElemInputId_KeyDash;
        case KEY_EQUAL: return ElemInputId_KeyEquals;
        case KEY_BACKSPACE: return ElemInputId_KeyBackspace;
        case KEY_TAB: return ElemInputId_KeyTab;
        case KEY_Q: return ElemInputId_KeyQ;
        case KEY_W: return ElemInputId_KeyW;
        case KEY_E: return ElemInputId_KeyE;
        case KEY_R: return ElemInputId_KeyR;
        case KEY_T: return ElemInputId_KeyT;
        case KEY_Y: return ElemInputId_KeyY;
        case KEY_U: return ElemInputId_KeyU;
        case KEY_I: return ElemInputId_KeyI;
        case KEY_O: return ElemInputId_KeyO;
        case KEY_P: return ElemInputId_KeyP;
        case KEY_LEFTBRACE: return ElemInputId_KeyLeftBrace;
        case KEY_RIGHTBRACE: return ElemInputId_KeyRightBrace;
        case KEY_BACKSLASH: return ElemInputId_KeyBackSlash;
        case KEY_CAPSLOCK: return ElemInputId_KeyCapsLock;
        case KEY_A: return ElemInputId_KeyA;
        case KEY_S: return ElemInputId_KeyS;
        case KEY_D: return ElemInputId_KeyD;
        case KEY_F: return ElemInputId_KeyF;
        case KEY_G: return ElemInputId_KeyG;
        case KEY_H: return ElemInputId_KeyH;
        case KEY_J: return ElemInputId_KeyJ;
        case KEY_K: return ElemInputId_KeyK;
        case KEY_L: return ElemInputId_KeyL;
        case KEY_SEMICOLON: return ElemInputId_KeySemiColon;
        case KEY_APOSTROPHE: return ElemInputId_KeyApostrophe;
        case KEY_ENTER: return ElemInputId_KeyEnter;
        case KEY_LEFTSHIFT: return ElemInputId_KeyLeftShift;
        case KEY_Z: return ElemInputId_KeyZ;
        case KEY_X: return ElemInputId_KeyX;
        case KEY_C: return ElemInputId_KeyC;
        case KEY_V: return ElemInputId_KeyV;
        case KEY_B: return ElemInputId_KeyB;
        case KEY_N: return ElemInputId_KeyN;
        case KEY_M: return ElemInputId_KeyM;
        case KEY_COMMA: return ElemInputId_KeyComma;
        case KEY_DOT: return ElemInputId_KeyPeriod;
        case KEY_SLASH: return ElemInputId_KeySlash;
        case KEY_RIGHTSHIFT: return ElemInputId_KeyRightShift;
        case KEY_LEFTCTRL: return ElemInputId_KeyLeftControl;
        case KEY_LEFTALT: return ElemInputId_KeyLeftAlt;
        case KEY_SPACE: return ElemInputId_KeySpacebar;
        case KEY_RIGHTALT: return ElemInputId_KeyRightAlt;
        case KEY_RIGHTCTRL: return ElemInputId_KeyRightControl;
        case KEY_INSERT: return ElemInputId_KeyInsert;
        case KEY_DELETE: return ElemInputId_KeyDelete;
        case KEY_LEFT: return ElemInputId_KeyLeftArrow;
        case KEY_HOME: return ElemInputId_KeyHome;
        case KEY_END: return ElemInputId_KeyEnd;
        case KEY_UP: return ElemInputId_KeyUpArrow;
        case KEY_DOWN: return ElemInputId_KeyDownArrow;
        case KEY_PAGEUP: return ElemInputId_KeyPageUp;
        case KEY_PAGEDOWN: return ElemInputId_KeyPageDown;
        case KEY_RIGHT: return ElemInputId_KeyRightArrow;
        case KEY_NUMLOCK: return ElemInputId_KeyNumpadLock;
        case KEY_KP7: return ElemInputId_KeyNumpad7;
        case KEY_KP4: return ElemInputId_KeyNumpad4;
        case KEY_KP1: return ElemInputId_KeyNumpad1;
        case KEY_KPSLASH: return ElemInputId_KeyNumpadDivide;
        case KEY_KP8: return ElemInputId_KeyNumpad8;
        case KEY_KP5: return ElemInputId_KeyNumpad5;
        case KEY_KP2: return ElemInputId_KeyNumpad2;
        case KEY_KP0: return ElemInputId_KeyNumpad0;
        case KEY_KPASTERISK: return ElemInputId_KeyNumpadMultiply;
        case KEY_KP9: return ElemInputId_KeyNumpad9;
        case KEY_KP6: return ElemInputId_KeyNumpad6;
        case KEY_KP3: return ElemInputId_KeyNumpad3;
        case KEY_KPDOT: return ElemInputId_KeyNumpadSeparator;
        case KEY_KPMINUS: return ElemInputId_KeyNumpadMinus;
        case KEY_KPPLUS: return ElemInputId_KeyNumpadAdd;
        case KEY_KPENTER: return ElemInputId_KeyNumpadEnter;
        case KEY_ESC: return ElemInputId_KeyEscape;
        case KEY_F1: return ElemInputId_KeyF1;
        case KEY_F2: return ElemInputId_KeyF2;
        case KEY_F3: return ElemInputId_KeyF3;
        case KEY_F4: return ElemInputId_KeyF4;
        case KEY_F5: return ElemInputId_KeyF5;
        case KEY_F6: return ElemInputId_KeyF6;
        case KEY_F7: return ElemInputId_KeyF7;
        case KEY_F8: return ElemInputId_KeyF8;
        case KEY_F9: return ElemInputId_KeyF9;
        case KEY_F10: return ElemInputId_KeyF10;
        case KEY_F11: return ElemInputId_KeyF11;
        case KEY_F12: return ElemInputId_KeyF12;
        case KEY_PRINT: return ElemInputId_KeyPrintScreen;
        case KEY_SCROLLLOCK: return ElemInputId_KeyScrollLock;
        case KEY_PAUSE: return ElemInputId_KeyPause;
        case KEY_LEFTMETA: return ElemInputId_KeyLeftSystem;
        case KEY_RIGHTMETA: return ElemInputId_KeyRightSystem;
        case KEY_COMPOSE: return ElemInputId_KeyApp;
        case BTN_LEFT: return ElemInputId_MouseLeftButton;
        case BTN_MIDDLE: return ElemInputId_MouseMiddleButton;
        case BTN_RIGHT: return ElemInputId_MouseRightButton;
        case BTN_SIDE: return ElemInputId_MouseExtraButton1;
        case BTN_EXTRA: return ElemInputId_MouseExtraButton2;
        default: return ElemInputId_Unknown;
    };

    return {};
}

void WaylandKeyboardKeyHandler(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    auto elapsedSeconds = (double)(SystemPlatformGetHighPerformanceCounter() - WaylandPerformanceCounterStart) / WaylandPerformanceCounterFrequencyInSeconds;
    auto inputDevice = AddWaylandInputDevice((void*)keyboard, ElemInputDeviceType_Keyboard);

    AddInputEvent({
        .Window = (ElemWindow)data,
        .InputDevice = inputDevice,
        .InputId = GetWaylandInputIdFromKeyCode(key),
        .InputType = ElemInputType_Digital,
        .Value = state == WL_KEYBOARD_KEY_STATE_PRESSED ? 1.0f : 0.0f,
        .ElapsedSeconds = elapsedSeconds
    });

    if (key == KEY_F11 && state == WL_KEYBOARD_KEY_STATE_RELEASED)
    {
        ElemSetWindowState((ElemWindow)data, ElemWindowState_FullScreen);
    }
}

wl_surface* WaylandCurrentPointerSurface = nullptr;

void WaylandUpdateCursor(ElemWindow window, wl_pointer* pointer, uint32_t serial)
{
    auto windowData = GetWaylandWindowData(window);
    SystemAssert(windowData);

    auto windowDataFull = GetWaylandWindowDataFull(window);
    SystemAssert(windowDataFull);

    if (windowData->WaylandSurface == WaylandCurrentPointerSurface)
    {
        if (!windowDataFull->IsCursorHidden)
        {
            WaylandSetDefaultCursor(pointer, serial);
        }
        else
        {
            wl_pointer_set_cursor(pointer, serial, nullptr, 0, 0);
        } 
    }
    else 
    {
        WaylandSetDefaultCursor(pointer, serial);
    }
}


void WaylandPointerEnterHandler(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy)
{
    WaylandCurrentPointerSurface = surface;
    WaylandUpdateCursor((ElemWindow)data, pointer, serial);
}

void WaylandPointerLeaveHandler(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface)
{
    WaylandCurrentPointerSurface = nullptr;
}

void WaylandPointerMotionHandler(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) 
{
    WaylandUpdateCursor((ElemWindow)data, pointer, 0);

    auto windowData = GetWaylandWindowData((ElemWindow)data);
    SystemAssert(windowData);

    windowData->SurfaceCursorPositionX = (uint32_t)wl_fixed_to_double(sx);
    windowData->SurfaceCursorPositionY = (uint32_t)wl_fixed_to_double(sy);
}

void WaylandPointerButtonHandler(void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    auto elapsedSeconds = (double)(SystemPlatformGetHighPerformanceCounter() - WaylandPerformanceCounterStart) / WaylandPerformanceCounterFrequencyInSeconds;
    auto inputDevice = AddWaylandInputDevice((void*)pointer, ElemInputDeviceType_Mouse);

    auto windowData = GetWaylandWindowData((ElemWindow)data);
    SystemAssert(windowData);

    if ((state == WL_POINTER_BUTTON_STATE_PRESSED && windowData->WaylandSurface == WaylandCurrentPointerSurface) || state != WL_POINTER_BUTTON_STATE_PRESSED)
    {
        // TODO: Check the order of the extra buttons
        
        AddInputEvent({
            .Window = (ElemWindow)data,
            .InputDevice = inputDevice,
            .InputId = GetWaylandInputIdFromKeyCode(button),
            .InputType = ElemInputType_Digital,
            .Value = state == WL_POINTER_BUTTON_STATE_PRESSED ? 1.0f : 0.0f,
            .ElapsedSeconds = elapsedSeconds
        });
    }
}

void WaylandPointerAxisHandler(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Inputs, "Pointer axis %d", value);

    // TODO: Axis
}

void WaylandRelativePointerMotionHandler(void* data, zwp_relative_pointer_v1* pointer, uint32_t time_hi, uint32_t time_lo, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t dx_unaccel, wl_fixed_t dy_unaccel)
{
    auto window = (ElemWindow)data;
    auto elapsedSeconds = (double)(SystemPlatformGetHighPerformanceCounter() - WaylandPerformanceCounterStart) / WaylandPerformanceCounterFrequencyInSeconds;
    auto inputDevice = AddWaylandInputDevice((void*)WaylandPointer, ElemInputDeviceType_Mouse);

    int32_t deltaX = wl_fixed_to_double(dx);
    int32_t deltaY = wl_fixed_to_double(dy);

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
}

void WaylandSeatCapabilitiesHandler(void* data, wl_seat* seat, uint32_t capabilities)
{
    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !waylandKeyboard) 
    {
        waylandKeyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(waylandKeyboard, &WaylandKeyboardListener, data);
    }

    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !WaylandPointer) 
    {
        WaylandPointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(WaylandPointer, &WaylandPointerListener, data);

        if (waylandRelativePointerManager) 
        {
            waylandRelativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(waylandRelativePointerManager, WaylandPointer);
            zwp_relative_pointer_v1_add_listener(waylandRelativePointer, &WaylandRelativePointerListener, data);
        }
    }
}

void WaylandInputRegistryGlobalHandler(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
    if (SystemFindSubString(interface, wl_seat_interface.name) != -1) 
    {
        waylandSeat = (wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(waylandSeat, &WaylandSeatListener, data);
    }
    else if (SystemFindSubString(interface, zwp_relative_pointer_manager_v1_interface.name) != -1) 
    {
        waylandRelativePointerManager = (zwp_relative_pointer_manager_v1*)wl_registry_bind(registry, name, &zwp_relative_pointer_manager_v1_interface, 1);
    }
}

void InitWaylandInputs(ElemWindow window)
{
    SystemAssert(window != ELEM_HANDLE_NULL);
    waylandInputDeviceDictionary = SystemCreateDictionary<void*, ElemInputDevice>(ApplicationMemoryArena, MAX_INPUT_DEVICES);

    auto registry = wl_display_get_registry(WaylandDisplay);

    wl_registry_add_listener(registry, &WaylandInputRegistryListener, (void*)window);
    wl_display_roundtrip(WaylandDisplay);
}
