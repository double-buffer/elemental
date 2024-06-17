#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Rename to resource tests
// TODO: Test barriers with compute/render encoders and different parameters

// TODO: Test Buffer create with get resource info and compare
// TODO: Test Reosurce UAV only if resource created with uav

// TODO: Test GetDataSpan buffer only

UTEST(Resource, CreateTexture2D) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();

    // Act

    // Assert
    ASSERT_FALSE(testHasLogErrors);
}

// TODO: Test for RTV valid only if the texture was created with the correct options
