#include "Elemental.h"
#include "ApplicationTests.h"
#include "utest.h"

UTEST(Window, CreateWindow) 
{
    // Arrange
    auto width = 640u;
    auto height = 480u;

    InitLog();
    auto application = ElemCreateApplication("TestApplication");

    // Act
    ElemWindowOptions options = { .Width = width, .Height = height };
    auto window = ElemCreateWindow(application, &options); 

    // Assert
    auto size = ElemGetWindowRenderSize(window);

    ElemFreeWindow(window);
    ElemFreeApplication(application);

    ASSERT_NE(0.0f, size.UIScale);
    ASSERT_EQ(width * size.UIScale, size.Width);
    ASSERT_EQ(height * size.UIScale, size.Height);
    ASSERT_EQ(ElemWindowState_Normal, size.WindowState);
}

UTEST(Window, CreateWindowWithState) 
{
    // Arrange
    InitLog();
    auto application = ElemCreateApplication("TestApplication");

    // Act
    ElemWindowOptions options = { .WindowState = ElemWindowState_Maximized };
    auto window = ElemCreateWindow(application, &options); 

    // Assert
    auto size = ElemGetWindowRenderSize(window);

    ElemFreeWindow(window);
    ElemFreeApplication(application);

    ASSERT_EQ(ElemWindowState_Maximized, size.WindowState);
}

// TODO: Only run those tests for Windows/MacOS/Linux
struct Window_SetWindowState
{
    ElemWindowState SourceState;
    ElemWindowState DestinationState;
};

UTEST_F_SETUP(Window_SetWindowState) 
{
}

UTEST_F_TEARDOWN(Window_SetWindowState) 
{
    // Arrange
    auto width = 640u;
    auto height = 480u;

    InitLog();
    auto application = ElemCreateApplication("TestApplication");
    ElemWindowOptions options = { .Width = width, .Height = height, .WindowState = utest_fixture->SourceState };
    auto window = ElemCreateWindow(application, &options); 
    ElemRunApplication(application, TestIterationRunHandler);

    // Act
    ElemSetWindowState(window, utest_fixture->DestinationState);
    ElemRunApplication(application, TestIterationRunHandler);

    // Assert
    auto size = ElemGetWindowRenderSize(window);    

    ElemFreeWindow(window);
    ElemFreeApplication(application);

    ASSERT_EQ(utest_fixture->DestinationState, size.WindowState);

    if (utest_fixture->DestinationState == ElemWindowState_Normal)
    {
        ASSERT_EQ(width * size.UIScale, size.Width);
        ASSERT_EQ(height * size.UIScale, size.Height);
    }
    else if (utest_fixture->DestinationState == ElemWindowState_Minimized)
    {
        ASSERT_EQ(0u, size.Width);
        ASSERT_EQ(0u, size.Height);
    }
}

UTEST_F(Window_SetWindowState, Normal_FullScreen) 
{
    utest_fixture->SourceState = ElemWindowState_Normal;
    utest_fixture->DestinationState = ElemWindowState_FullScreen;
}

UTEST_F(Window_SetWindowState, Minimized_FullScreen) 
{
    utest_fixture->SourceState = ElemWindowState_Minimized;
    utest_fixture->DestinationState = ElemWindowState_FullScreen;
}

UTEST_F(Window_SetWindowState, Maximized_FullScreen) 
{
    utest_fixture->SourceState = ElemWindowState_Maximized;
    utest_fixture->DestinationState = ElemWindowState_FullScreen;
}

UTEST_F(Window_SetWindowState, Normal_Maximized) 
{
    utest_fixture->SourceState = ElemWindowState_Normal;
    utest_fixture->DestinationState = ElemWindowState_Maximized;
}

UTEST_F(Window_SetWindowState, FullScreen_Maximized) 
{
    utest_fixture->SourceState = ElemWindowState_FullScreen;
    utest_fixture->DestinationState = ElemWindowState_Maximized;
}

UTEST_F(Window_SetWindowState, Normal_Minimized) 
{
    utest_fixture->SourceState = ElemWindowState_Normal;
    utest_fixture->DestinationState = ElemWindowState_Minimized;
}

UTEST_F(Window_SetWindowState, Maximized_Minimized) 
{
    utest_fixture->SourceState = ElemWindowState_Maximized;
    utest_fixture->DestinationState = ElemWindowState_Minimized;
}

UTEST_F(Window_SetWindowState, Minimized_Normal) 
{
    utest_fixture->SourceState = ElemWindowState_Minimized;
    utest_fixture->DestinationState = ElemWindowState_Normal;
}

UTEST_F(Window_SetWindowState, FullScreen_Normal) 
{
    utest_fixture->SourceState = ElemWindowState_FullScreen;
    utest_fixture->DestinationState = ElemWindowState_Normal;
}

UTEST_F(Window_SetWindowState, Maximized_Normal) 
{
    utest_fixture->SourceState = ElemWindowState_Maximized;
    utest_fixture->DestinationState = ElemWindowState_Normal;
}
