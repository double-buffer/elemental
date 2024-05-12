#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

UTEST(Texture, CreateTexture) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();

    // Act

    // Assert
    ASSERT_FALSE(testHasLogErrors);
}

// TODO: Test for RTV valid only if the texture was created with the correct options
