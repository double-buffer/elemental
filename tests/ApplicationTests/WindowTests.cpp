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
    ASSERT_NE(0.0f, size.UIScale);
    ASSERT_EQ(width * size.UIScale, size.Width);
    ASSERT_EQ(height * size.UIScale, size.Height);
    ASSERT_EQ(ElemWindowState_Normal, size.WindowState);
}
