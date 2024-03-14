#pragma once

#include "Elemental.h"

#ifdef WIN32
    #define strcpy strcpy_s
#endif

extern bool testForceVulkanApi;

void InitLog()
{
    #ifdef _DEBUG
    ElemConfigureLogHandler(ElemConsoleLogHandler);
    ElemEnableGraphicsDebugLayer();
    #else
    ElemConfigureLogHandler(ElemConsoleErrorLogHandler);
    #endif
}
