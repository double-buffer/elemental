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
