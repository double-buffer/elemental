#pragma once

#include <functional>
#include "Elemental.h"

void InitLog()
{
    #ifdef _DEBUG
    ElemConfigureLogHandler(ElemConsoleLogHandler);
    #endif
}

struct ApplicationTestPayload 
{
    std::function<void()> TestFunction;
};

void ApplicationTestInitFunction(void* payload)
{
    auto applicationTestPayload = (ApplicationTestPayload*)payload;
    applicationTestPayload->TestFunction();

    ElemExitApplication();
}

void RunApplicationTest(std::function<void()> testFunction)
{
    InitLog();

    ApplicationTestPayload payload = { .TestFunction = testFunction };

    ElemRunApplicationParameters runParameters = 
    {
        .InitHandler = ApplicationTestInitFunction,
        .Payload = &payload
    };

    ElemRunApplication(&runParameters);
}

