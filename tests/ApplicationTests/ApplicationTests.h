#pragma once

#include "Elemental.h"

void InitLog()
{
    #ifdef _DEBUG
    ElemConfigureLogHandler(ElemConsoleLogHandler);
    #endif
}

int32_t counter = 0;

bool TestIterationRunHandler(ElemApplicationStatus status)
{
    if (counter > 128 || status == ElemApplicationStatus_Closing)
    {
        counter = 0;
        printf("Closing\n");
        return false;
    }

    #ifdef WIN32
        Sleep(5);
    #else
        usleep(5000);
    #endif

    counter++;
    return true;
}
