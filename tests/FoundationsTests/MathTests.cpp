#include "SystemFunctions.h"
#include "utest.h"

UTEST(MathFunctions, SystemRoundUpToPowerOf2)
{
    // Arrange
    auto testNumber = 45;

    // Act
    auto result = SystemRoundUpToPowerOf2(testNumber);

    // Assert
    ASSERT_EQ_MSG((size_t)64, result, "SystemRoundUpToPowerOf2 returned an invalid power-of-two value.");
}

UTEST(MathFunctions, SystemRound)
{
    // Arrange
    auto testNumber = 45.67f;

    // Act
    auto result = SystemRound(testNumber);

    // Assert
    ASSERT_EQ_MSG(46.0, result, "SystemRound returned an invalid rounded value.");
}

UTEST(MathFunctions, SystemRoundUpNearZero)
{
    // Arrange
    auto testNumber = 0.1;

    // Act
    auto result = SystemRoundUp(testNumber);

    // Assert
    ASSERT_EQ_MSG(1, result, "SystemRoundUp should round a positive fractional value to the next integer.");
}

UTEST(MathFunctions, SystemRoundUpExact)
{
    // Arrange
    auto testNumber = 6.0;

    // Act
    auto result = SystemRoundUp(testNumber);

    // Assert
    ASSERT_EQ_MSG(6, result, "SystemRoundUp should preserve an exact integer value.");
}

UTEST(MathFunctions, SystemAbs)
{
    // Arrange
    auto testNumber = -65;

    // Act
    auto result = SystemAbs(testNumber);

    // Assert
    ASSERT_EQ_MSG(65, result, "SystemAbs returned an invalid absolute value.");
}

UTEST(MathFunctions, SystemMax)
{
    // Arrange / Act
    auto result = SystemMax(67, 54);

    // Assert
    ASSERT_EQ_MSG(67, result, "SystemMax did not return the greater value.");
}

UTEST(MathFunctions, SystemMin)
{
    // Arrange / Act
    auto result = SystemMin(67, 54);

    // Assert
    ASSERT_EQ_MSG(54, result, "SystemMin did not return the lesser value.");
}
