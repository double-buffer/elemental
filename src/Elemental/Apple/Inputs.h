#pragma once

#include "Elemental.h"

enum AppleGamepadDirection
{
    LeftStick,
    RightStick,
    Dpad,
    MouseWheel
};

void InitInputs(ElemWindow window);
void TouchHandler(ElemWindow window, void* deviceId, uint32_t fingerIndex, float x, float y, float deltaX, float deltaY, uint32_t state);
