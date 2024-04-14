#pragma once

#include "Elemental.h"

void InitLog()
{
    #ifdef _DEBUG
    ElemConfigureLogHandler(ElemConsoleLogHandler);
    #endif
}

