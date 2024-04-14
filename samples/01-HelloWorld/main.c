#include "Elemental.h"

int32_t counter = 0;

void InitSample(void* payload)
{
    printf("Hello World!\n");
}

int main(void)
{
    ElemConfigureLogHandler(ElemConsoleLogHandler);
    
    ElemRunApplication(&(ElemRunApplicationParameters)
    {
        .ApplicationName = "Hello World",
        .InitHandler = InitSample,
    });

    return 0;
}
