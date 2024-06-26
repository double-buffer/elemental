#include "SystemFunctions.h"
#include "utest.h"

UTEST(MathFunctions, SystemRoundUpToPowerOf2) 
{
    // Arrange
    auto testNumber = 45;
    
    // Act
    auto result = SystemRoundUpToPowerOf2(testNumber);

    // Assert
    ASSERT_EQ((size_t)64, result);
}

UTEST(MathFunctions, SystemRound) 
{
    // Arrange
    auto testNumber = 45.67f;
    
    // Act
    auto result = SystemRound(testNumber);

    // Assert
    ASSERT_EQ(46.0, result);
}

UTEST(MathFunctions, SystemRoundUpNearZero) 
{
    // Arrange
    auto testNumber = 0.1;
    
    // Act
    auto result = SystemRoundUp(testNumber);

    // Assert
    ASSERT_EQ(1, result);
}

UTEST(MathFunctions, SystemRoundUpExact) 
{
    // Arrange
    auto testNumber = 6.0;
    
    // Act
    auto result = SystemRoundUp(testNumber);

    // Assert
    ASSERT_EQ(6, result);
}

UTEST(MathFunctions, SystemAbs) 
{
    // Arrange
    auto testNumber = -65;
    
    // Act
    auto result = SystemAbs(testNumber);

    // Assert
    ASSERT_EQ(65, result);
}

UTEST(MathFunctions, SystemMax) 
{
    // Arrange / Act
    auto result = SystemMax(67, 54);

    // Assert
    ASSERT_EQ(67, result);
}

UTEST(MathFunctions, SystemMin) 
{
    // Arrange / Act
    auto result = SystemMin(67, 54);

    // Assert
    ASSERT_EQ(54, result);
}
