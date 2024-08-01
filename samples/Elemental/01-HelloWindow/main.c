#include "Elemental.h"

typedef struct
{
    ElemWindow Window;
} ApplicationPayload;

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemWindow window = ElemCreateWindow(NULL);
    applicationPayload->Window = window;
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    ElemFreeWindow(applicationPayload->Window);
    printf("Exit Sample\n");
}

int main(void)
{
    ElemConfigureLogHandler(ElemConsoleLogHandler);

    ApplicationPayload payload = {0};

    ElemRunApplication(&(ElemRunApplicationParameters)
    {
        .ApplicationName = "Hello Window",
        .InitHandler = InitSample,
        .FreeHandler = FreeSample,
        .Payload = &payload
    });
}
