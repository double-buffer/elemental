#include "SystemDictionary.h"
#include "SystemFunctions.h"
#include "utest.h"

UTEST(Dictionary, AddValue) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<char>, int32_t>(stackMemoryArena, 24);
    
    // Act
    for (size_t i = 0; i < 10; i++)
    {
        SystemAddDictionaryEntry(dictionary, SystemFormatString(stackMemoryArena, "Test%d", i), (int32_t)i);
    }
    
    auto testValue = dictionary["Test9"];

    // Assert
    ASSERT_EQ(9, testValue);
}

UTEST(Dictionary, AddValue_KeyStruct) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<int32_t, int32_t>(stackMemoryArena, 24);
    
    // Act
    for (size_t i = 0; i < 10; i++)
    {
        SystemAddDictionaryEntry(dictionary, (int32_t)i, (int32_t)i);
    }
    
    auto testValue = dictionary[9];

    // Assert
    ASSERT_EQ(9, testValue);
}

UTEST(Dictionary, RemoveValue) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<char>, int32_t>(stackMemoryArena, 24);
    
    // Act
    for (size_t i = 0; i < 10; i++)
    {
        SystemAddDictionaryEntry(dictionary, SystemFormatString(stackMemoryArena, "Test%d", i), (int32_t)i);
    }
    
    SystemRemoveDictionaryEntry(dictionary, "Test9");
    auto testValue = dictionary["Test9"];

    // Assert
    ASSERT_EQ(0, testValue);
}

UTEST(Dictionary, RemoveValue_KeyStruct) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<int32_t, int32_t>(stackMemoryArena, 24);
    
    // Act
    for (size_t i = 0; i < 10; i++)
    {
        SystemAddDictionaryEntry(dictionary, (int32_t)i, (int32_t)i);
    }
    
    SystemRemoveDictionaryEntry(dictionary, 9);
    auto testValue = dictionary[9];

    // Assert
    ASSERT_EQ(0, testValue);
}

UTEST(Dictionary, GrowStorage) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<char>, int32_t>(stackMemoryArena, 24);
    
    for (size_t i = 0; i < 10; i++)
    {
        SystemAddDictionaryEntry(dictionary, SystemFormatString(stackMemoryArena, "Test%d", i), (int32_t)i);
    }

    SystemRemoveDictionaryEntry(dictionary, "Test2");
    SystemRemoveDictionaryEntry(dictionary, "Test8");

    SystemAddDictionaryEntry(dictionary, "TestOneMore", 28);
    SystemAddDictionaryEntry(dictionary, "TestOneMore2", 29);
    SystemAddDictionaryEntry(dictionary, "TestOneMore3", 30);
    SystemAddDictionaryEntry(dictionary, "TestOneMore4", 31);
    SystemAddDictionaryEntry(dictionary, "TestOneMore5", 32);

    // Act
    SystemAddDictionaryEntry(dictionary, "TestOneMore6", 33);
    SystemRemoveDictionaryEntry(dictionary, "TestOneMore");
    SystemRemoveDictionaryEntry(dictionary, "TestOneMore2");
    SystemRemoveDictionaryEntry(dictionary, "TestOneMore6");

    auto testValue = dictionary["TestOneMore5"];

    // Assert
    ASSERT_EQ(32, testValue);
    DebugDictionary(dictionary);
}
