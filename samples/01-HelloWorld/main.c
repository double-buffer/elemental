#include <stdio.h>
#include "Elemental.h"

int32_t counter = 0;

// TODO: Provide a default handler that can be used out of the box for other samples. We show how to provide one here on the first one
void LogMessageHandler(LogMessageType messageType, LogMessageCategory category, ReadOnlySpan<char> function, ReadOnlySpan<char> message) 
{
    printf("[");
    printf("\033[36m");

    // TODO: Provide a mapping function
    if (category == LogMessageCategory_Memory)
    {
        printf("Memory");
    }
    else if (category == LogMessageCategory_NativeApplication)
    {
        printf("Application");
    }
    else if (category == LogMessageCategory_Graphics)
    {
        printf("Graphics");
    }
    else if (category == LogMessageCategory_Inputs)
    {
        printf("Inputs");
    }

    printf("\033[0m]");

    printf("\033[32m %s", function.Pointer);

    if (messageType == LogMessageType_Error)
    {
        printf("\033[31m");
    }
    else if (messageType == LogMessageType_Warning)
    {
        printf("\033[33m");
    }
    else
    {
        printf("\033[0m");
    }

    printf(" %s\n\033[0m", message.Pointer);
}

bool RunHandler(elemApplicationStatus status)
{
    if (counter > 10 || !status.IsActive)
    {
        return false;
    }

    printf("Hello World {counter}!", counter++);
    return true;
}

void main()
{
    // Maybe
    ElemConfigureLogHandler(LogMessageHandler);
    // ElemConfigureLogHandler(emConsoleLogHandler);

    auto application = ElemCreateApplication("Hello World");
    ElemRunApplication(application, RunHandler);
    ElemFreeApplication(application);

    // Actual
    auto applicationService = elemInitApplicationService({ LogMessageHandler = LogMessageHandler });
    auto application = elemCreateApplication("Hello World");

    elemRunApplication(application, RunHandler);

    elemFreeApplication(application);
    elemFreeApplicationService(applicationService);
}
