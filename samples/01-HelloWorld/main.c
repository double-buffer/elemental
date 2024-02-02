#include "Elemental.h"

int32_t counter = 0;

bool RunHandler(ElemApplicationStatus status)
{
    if (counter > 10)// || !status.IsActive)
    {
        return false;
    }

    printf("Hello World %d!\n", counter++);
    return true;
}

int main()
{
    ElemConfigureLogHandler(ElemConsoleLogHandler);
    
    ElemApplication application = ElemCreateApplication("Hello World");
    ElemRunApplication(application, RunHandler);
    ElemFreeApplication(application);

    return 0;
}
