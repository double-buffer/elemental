#include "utest.h"
#include "SystemFunctions.h"

UTEST(StringFunctions, SystemConvertNumberToString)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto testNumber = 45;

    // Act
    auto result = SystemConvertNumberToString(stackMemoryArena, testNumber);

    // Assert
    ASSERT_STREQ_MSG("45", result.Pointer, "Integer to string conversion returned invalid text.");
}

UTEST(StringFunctions, SystemConvertFloatToString)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto testNumber = 45.76;

    // Act
    auto result = SystemConvertFloatToString(stackMemoryArena, testNumber);

    // Assert
    ASSERT_STREQ_MSG("45.76", result.Pointer, "Floating-point to string conversion returned invalid text.");
}

UTEST(StringFunctions, SystemFormatString)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();

    // Act
    auto result = SystemFormatString(stackMemoryArena, "This is a test: %s, number: %d, float: %f end of the test", "TestString", -54, -23.89f);

    // Assert
    ASSERT_STREQ_MSG("This is a test: TestString, number: -54, float: -23.89 end of the test", result.Pointer, "Formatted string content is invalid.");
}

UTEST(StringFunctions, SystemSplitString)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();

    // Act
    auto result = SystemSplitString(stackMemoryArena, "Test/Split/String", '/');

    // Assert
    ASSERT_EQ_MSG(3, (int32_t)result.Length, "String split returned an invalid number of parts.");
    ASSERT_STREQ_MSG("Test", result[0].Pointer, "String split first part is invalid.");
    ASSERT_STREQ_MSG("Split", result[1].Pointer, "String split second part is invalid.");
    ASSERT_STREQ_MSG("String", result[2].Pointer, "String split third part is invalid.");
}

UTEST(StringFunctions, SystemLastIndexOf)
{
    // Act
    auto result = SystemLastIndexOf("Test/String/", '/');

    // Assert
    ASSERT_EQ_MSG(11, result, "SystemLastIndexOf returned an invalid separator position.");
}

UTEST(StringFunctions, SystemFindSubString_Found)
{
    // Act
    auto result = SystemFindSubString("Test/String/", "String");

    // Assert
    ASSERT_EQ_MSG(5, result, "SystemFindSubString returned an invalid substring position.");
}

UTEST(StringFunctions, SystemFindSubString_NotFound)
{
    // Act
    auto result = SystemFindSubString("Test/String/", "Sutoringu");

    // Assert
    ASSERT_EQ_MSG(-1, result, "SystemFindSubString should return -1 when the substring is absent.");
}

UTEST(StringFunctions, SystemConvertUtf8ToWideChar)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto testString = "Test String éé";

    // Act
    auto testStringWide = SystemConvertUtf8ToWideChar(stackMemoryArena, testString);
    auto result = SystemConvertWideCharToUtf8(stackMemoryArena, testStringWide);

    // Assert
    ASSERT_STREQ_MSG(testString, result.Pointer, "UTF-8 to wide-character roundtrip did not preserve the source string.");
}
