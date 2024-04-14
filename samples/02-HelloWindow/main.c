#include "Elemental.h"

typedef struct
{
    ElemWindow Window;
} ApplicationPayload;

const char* GetWindowStateLabel(ElemWindowState state)
{
    switch (state)
    {
        case ElemWindowState_FullScreen:
            return "FullScreen";
        case ElemWindowState_Maximized:
            return "Maximized";
        case ElemWindowState_Minimized:
            return "Minimized";
        case ElemWindowState_Normal:
            return "Normal";
    }

    return "Unknown";
}

void InitSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;

    ElemWindow window = ElemCreateWindow(NULL);
    applicationPayload->Window = window;
}

void FreeSample(void* payload)
{
    ApplicationPayload* applicationPayload = (ApplicationPayload*)payload;
    ElemFreeWindow( applicationPayload->Window);
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
