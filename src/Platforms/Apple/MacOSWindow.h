#pragma once
#include "PreCompiledHeader.h"

struct MacOSWindow
{
    NS::Window* WindowHandle = nullptr;
    //WINDOWPLACEMENT WindowPlacement;
    uint32_t Width;
    uint32_t Height;
    float_t UIScale;
};