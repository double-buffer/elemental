#include "Elemental.h"

static ElemWindow globalWindow;

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

bool RunHandler(ElemApplicationStatus status)
{
    if (status == ElemApplicationStatus_Closing)
    {
        printf("Closing Application...\n");
        return false;
    }

    ElemWindowSize renderSize = ElemGetWindowRenderSize(globalWindow);

    char temp[255];
    sprintf(temp, "Hello Window! (Current RenderSize: Width=%d, Height=%d, UIScale=%.2f, State=%s)", renderSize.Width, renderSize.Height, renderSize.UIScale, GetWindowStateLabel(renderSize.WindowState));
    ElemSetWindowTitle(globalWindow, temp);

    #ifdef WIN32
        Sleep(5);
    #else
        usleep(5000);
    #endif

    return true;
}

int main(void)
{
    ElemConfigureLogHandler(ElemConsoleLogHandler);
    
    ElemApplication application = ElemCreateApplication("Hello World");
    globalWindow = ElemCreateWindow(application, &(ElemWindowOptions) { .Title = "Hello Window!" });

    ElemRunApplication(application, RunHandler);
    ElemFreeApplication(application);

    return 0;
}
