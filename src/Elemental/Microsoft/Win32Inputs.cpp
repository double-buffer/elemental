#include "Win32Inputs.h"
#include "Inputs.h"

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
        case VK_ESCAPE: return ElemInputId_Escape;
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
