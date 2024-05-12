#include "Win32Inputs.h"
#include "Inputs/Inputs.h"

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
        case VK_BACK: return ElemInputId_Back;
        case VK_TAB: return ElemInputId_Tab;
        case VK_CLEAR: return ElemInputId_Clear;
        case VK_RETURN: return ElemInputId_Return;
        case VK_CONTROL: return ElemInputId_Control;
        case VK_APPS: return ElemInputId_Menu;
        case VK_PAUSE: return ElemInputId_Pause;
        case VK_CAPITAL: return ElemInputId_CapsLock;
        case VK_ESCAPE: return ElemInputId_Escape;
        case VK_SPACE: return ElemInputId_Space;
        case VK_PRIOR: return ElemInputId_PageUp;
        case VK_NEXT: return ElemInputId_PageDown;
        case VK_END: return ElemInputId_End;
        case VK_HOME: return ElemInputId_Home;
        case VK_LEFT: return ElemInputId_Left;
        case VK_UP: return ElemInputId_Up;
        case VK_RIGHT: return ElemInputId_Right;
        case VK_DOWN: return ElemInputId_Down;
        case VK_EXECUTE: return ElemInputId_Execute;
        case VK_SNAPSHOT: return ElemInputId_PrintScreen;
        case VK_INSERT: return ElemInputId_Insert;
        case VK_DELETE: return ElemInputId_Delete;
        case VK_LWIN: return ElemInputId_LeftSystemButton;
        case VK_RWIN: return ElemInputId_RightSystemButton;
        case VK_NUMPAD0: return ElemInputId_Numpad0;
        case VK_NUMPAD1: return ElemInputId_Numpad1;
        case VK_NUMPAD2: return ElemInputId_Numpad2;
        case VK_NUMPAD3: return ElemInputId_Numpad3;
        case VK_NUMPAD4: return ElemInputId_Numpad4;
        case VK_NUMPAD5: return ElemInputId_Numpad5;
        case VK_NUMPAD6: return ElemInputId_Numpad6;
        case VK_NUMPAD7: return ElemInputId_Numpad7;
        case VK_NUMPAD8: return ElemInputId_Numpad8;
        case VK_NUMPAD9: return ElemInputId_Numpad9;
        case VK_MULTIPLY: return ElemInputId_Multiply;
        case VK_ADD: return ElemInputId_Add;
        case VK_SEPARATOR: return ElemInputId_Separator;
        case VK_SUBTRACT: return ElemInputId_Subtract;
        case VK_DECIMAL: return ElemInputId_Decimal;
        case VK_DIVIDE: return ElemInputId_Divide;
        case VK_F1: return ElemInputId_F1;
        case VK_F2: return ElemInputId_F2;
        case VK_F3: return ElemInputId_F3;
        case VK_F4: return ElemInputId_F4;
        case VK_F5: return ElemInputId_F5;
        case VK_F6: return ElemInputId_F6;
        case VK_F7: return ElemInputId_F7;
        case VK_F8: return ElemInputId_F8;
        case VK_F9: return ElemInputId_F9;
        case VK_F10: return ElemInputId_F10;
        case VK_F11: return ElemInputId_F11;
        case VK_F12: return ElemInputId_F12;
        case VK_LSHIFT: return ElemInputId_LeftShift;
        case VK_RSHIFT: return ElemInputId_RightShift;
        case VK_LCONTROL: return ElemInputId_LeftControl;
        case VK_RCONTROL: return ElemInputId_RightControl;
        case VK_LMENU: return ElemInputId_LeftAlt;
        case VK_RMENU: return ElemInputId_RightAlt;
        default: return ElemInputId_Unknown;
    };
}

void ProcessWin32KeyboardInput(ElemWindow window, UINT message, WPARAM wParam, LPARAM lParam)
{
    ElemInputEvent event =
    {
        .Window = window,
        .InputId = GetWin32InputIdFromKeyCode(wParam),
        .Value = (message == WM_KEYDOWN) ? 1.0f : 0.0f
    };

    AddInputEvent(event);
}
