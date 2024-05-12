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
    ASSERT_STREQ("45", result.Pointer);
}

UTEST(StringFunctions, SystemConvertFloatToString) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto testNumber = 45.76;
    
    // Act
    auto result = SystemConvertFloatToString(stackMemoryArena, testNumber);

    // Assert
    ASSERT_STREQ("45.76", result.Pointer);
}

UTEST(StringFunctions, SystemFormatString) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    // Act
    auto result = SystemFormatString(stackMemoryArena, "This is a test: %s, number: %d, float: %f end of the test", "TestString", -54, -23.89f);

    // Assert
    ASSERT_STREQ("This is a test: TestString, number: -54, float: -23.89 end of the test", result.Pointer);
}

UTEST(StringFunctions, SystemSplitString) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    // Act
    auto result = SystemSplitString(stackMemoryArena, "Test/Split/String", '/');

    // Assert
    ASSERT_EQ(3, (int32_t)result.Length);
    ASSERT_STREQ("Test", result[0].Pointer);
    ASSERT_STREQ("Split", result[1].Pointer);
    ASSERT_STREQ("String", result[2].Pointer);
}

UTEST(StringFunctions, SystemLastIndexOf) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    // Act
    auto result = SystemLastIndexOf("Test/String/", '/');

    // Assert
    ASSERT_EQ(11, result);
}

UTEST(StringFunctions, SystemFindSubString_Found) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    // Act
    auto result = SystemFindSubString("Test/String/", "String");

    // Assert
    ASSERT_EQ(5, result);
}

UTEST(StringFunctions, SystemFindSubString_NotFound) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    // Act
    auto result = SystemFindSubString("Test/String/", "Sutoringu");

    // Assert
    ASSERT_EQ(-1, result);
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
    ASSERT_STREQ(testString, result.Pointer);
}
