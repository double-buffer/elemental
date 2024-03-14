#pragma once

#include "Elemental.h"

extern bool testForceVulkanApi;

void InitLog()
{
    #ifdef _DEBUG
    ElemConfigureLogHandler(ElemConsoleLogHandler);
    #else
    ElemConfigureLogHandler(ElemConsoleErrorLogHandler);
    #endif
}