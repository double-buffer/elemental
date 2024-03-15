#pragma once

#include "Elemental.h"

#ifdef WIN32
    #define strcpy strcpy_s
#endif

extern bool testForceVulkanApi;

void InitLog()
{
    ElemGraphicsOptions options = { .PreferVulkan = testForceVulkanApi };

    #ifdef _DEBUG
    ElemConfigureLogHandler(ElemConsoleLogHandler);
    options.EnableDebugLayer = true;
    #else
    ElemConfigureLogHandler(ElemConsoleErrorLogHandler);
    #endif

    ElemSetGraphicsOptions(&options);
}
