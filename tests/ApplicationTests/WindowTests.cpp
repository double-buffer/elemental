#include "Elemental.h"
#include "ApplicationTests.h"
#include "utest.h"

UTEST(Window, CreateWindow) 
{
    // Arrange
    auto width = 1280u;
    auto height = 720u;

    InitLog();
    auto application = ElemCreateApplication("TestApplication");

    // Act
    ElemWindowOptions options = { .Width = width, .Height = height };
    auto window = ElemCreateWindow(application, &options); 

    // Assert
    auto size = ElemGetWindowRenderSize(window);
    ASSERT_EQ(width * size.UIScale, size.Width);
    ASSERT_EQ(height * size.UIScale, size.Height);
    ASSERT_EQ(ElemWindowState_Normal, size.WindowState);
}
