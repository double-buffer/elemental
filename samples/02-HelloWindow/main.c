#include "Elemental.h"

static ElemWindow globalWindow;

bool RunHandler(ElemApplicationStatus status)
{
    if (status == ElemApplicationStatus_Closing)
    {
        printf("Closing Application...\n");
        return false;
    }

    ElemWindowSize renderSize = ElemGetWindowRenderSize(globalWindow);

    char temp[255];
    sprintf(temp, "Hello Window! (Current RenderSize: Width=%d, Height=%d, UIScale=%.2f)", renderSize.Width, renderSize.Height, renderSize.UIScale);
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
    globalWindow = ElemCreateWindow(application, &(ElemWindowOptions) { .Title = "Hello Window!", .WindowState = ElemWindowState_Maximized });

    // TODO: Allow to pass parameters to the run handler?
    ElemRunApplication(application, RunHandler);
    ElemFreeApplication(application);

    return 0;
}
