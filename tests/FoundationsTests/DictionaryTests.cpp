#include "SystemDictionary.h"
#include "SystemFunctions.h"
#include "utest.h"

struct DictionaryTestStruct
{
    int64_t Value1;
    int64_t Value2;
};

UTEST(Dictionary, AddValue)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<char>, int32_t>(stackMemoryArena, 24);

    // Act
    for (int32_t i = 0; i < 10; i++)
    {
        SystemAddDictionaryEntry(dictionary, SystemFormatString(stackMemoryArena, "Test%d", i), i);
    }

    // Assert
    auto testValue = dictionary["Test9"];
    ASSERT_EQ(9, testValue);
}

UTEST(Dictionary, AddValue_KeyStruct)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<int32_t, DictionaryTestStruct>(stackMemoryArena, 24);

    // Act
    for (int32_t i = 0; i < 10; i++)
    {
        DictionaryTestStruct testStruct = {};
        testStruct.Value1 = i;
        testStruct.Value2 = i * i;
        SystemAddDictionaryEntry(dictionary, i, testStruct);
    }

    // Assert
    auto testValue = dictionary[9];
    ASSERT_EQ(9, testValue.Value1);
    ASSERT_EQ(81, testValue.Value2);
}

UTEST(Dictionary, RemoveValue)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<char>, int32_t>(stackMemoryArena, 24);

    for (int32_t i = 0; i < 10; i++)
    {
        SystemAddDictionaryEntry(dictionary, SystemFormatString(stackMemoryArena, "Test%d", i), i);
    }

    // Act
    SystemRemoveDictionaryEntry(dictionary, "Test6");

    // Assert
    for (int32_t i = 0; i < 10; i++)
    {
        auto testValue = dictionary[SystemFormatString(stackMemoryArena, "Test%d", i)];

        if (i == 6)
        {
            ASSERT_EQ(0, testValue);
        }
        else
        {
            ASSERT_EQ(i, testValue);
        }
    }
}

UTEST(Dictionary, RemoveValueNoParent)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<char>, int32_t>(stackMemoryArena, 24);

    for (int32_t i = 0; i < 10; i++)
    {
        SystemAddDictionaryEntry(dictionary, SystemFormatString(stackMemoryArena, "Test%d", i), i);
    }

    // Act
    SystemRemoveDictionaryEntry(dictionary, "Test8");

    // Assert
    for (int32_t i = 0; i < 10; i++)
    {
        auto testValue = dictionary[SystemFormatString(stackMemoryArena, "Test%d", i)];

        if (i == 8)
        {
            ASSERT_EQ(0, testValue);
        }
        else
        {
            ASSERT_EQ(i, testValue);
        }
    }
}

UTEST(Dictionary, RemoveValue_KeyStruct)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<int32_t, int32_t>(stackMemoryArena, 24);

    for (int32_t i = 0; i < 10; i++)
    {
        SystemAddDictionaryEntry(dictionary, i, i);
    }

    // Act
    SystemRemoveDictionaryEntry(dictionary, 9);

    // Assert
    auto testValue = dictionary[9];
    ASSERT_EQ(0, testValue);
}

UTEST(Dictionary, GrowStorage)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<char>, int32_t>(stackMemoryArena, 24);

    for (int32_t i = 0; i < 10; i++)
    {
        SystemAddDictionaryEntry(dictionary, SystemFormatString(stackMemoryArena, "Test%d", i), i);
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

    // Assert
    auto testValue = dictionary["TestOneMore5"];
    ASSERT_EQ(32, testValue);
}

UTEST(Dictionary, NotEnoughStorage)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<char>, int32_t>(stackMemoryArena, 24);

    for (int32_t i = 0; i < 24; i++)
    {
        SystemAddDictionaryEntry(dictionary, SystemFormatString(stackMemoryArena, "Test%d", i), i);
    }

    // Act
    SystemAddDictionaryEntry(dictionary, "TestOneMore6", 33);

    // Assert
    auto testValue = dictionary["TestOneMore6"];
    ASSERT_EQ(0, testValue);
}

UTEST(Dictionary, RemoveValuesAfterFull)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<char>, int32_t>(stackMemoryArena, 24);

    for (int32_t i = 0; i < 24; i++)
    {
        SystemAddDictionaryEntry(dictionary, SystemFormatString(stackMemoryArena, "Test%d", i), i);
    }

    SystemRemoveDictionaryEntry(dictionary, "Test0");

    // Act
    SystemAddDictionaryEntry(dictionary, "TestNew", 28);

    // Assert
    auto testValue = dictionary["TestNew"];
    ASSERT_EQ(28, testValue);
}

UTEST(Dictionary, BigDictionary)
{
    // Arrange
    auto maxElements = 1000000;
    auto stackMemoryArena = SystemAllocateMemoryArena();
    auto memoryArena = SystemAllocateMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<char>, int32_t>(memoryArena, maxElements);

    // Act
    for (int32_t i = 0; i < 10000; i++)
    {
        SystemAddDictionaryEntry(dictionary, SystemFormatString(stackMemoryArena, "Test%d", i), i);
    }

    // Assert
    for (int32_t i = 0; i < 10000; i++)
    {
        auto testValue = dictionary[SystemFormatString(stackMemoryArena, "Test%d", i)];
        ASSERT_EQ(i, testValue);
    }
}

UTEST(Dictionary, ContainsKey)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<char>, int32_t>(stackMemoryArena, 24);

    for (int32_t i = 0; i < 10; i++)
    {
        SystemAddDictionaryEntry(dictionary, SystemFormatString(stackMemoryArena, "Test%d", i), i);
    }

    // Act
    auto testValue = SystemDictionaryContainsKey(dictionary, "Test9");

    // Assert
    ASSERT_TRUE(testValue);
}

UTEST(Dictionary, ContainsKey_KeyStruct)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<int32_t, int32_t>(stackMemoryArena, 24);

    for (int32_t i = 0; i < 10; i++)
    {
        SystemAddDictionaryEntry(dictionary, i, i);
    }

    // Act
    auto testValue = SystemDictionaryContainsKey(dictionary, 9);

    // Assert
    ASSERT_TRUE(testValue);
}

UTEST(Dictionary, ReadOnlySpanHashUsesAllBytes)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<uint32_t>, int32_t>(stackMemoryArena, 8);
    uint32_t key1[] = { 0x00001234, 1 };
    uint32_t key2[] = { 0x00001234, 2 };

    // Act
    SystemAddDictionaryEntry(dictionary, ReadOnlySpan<uint32_t>(key1, 2), 10);
    SystemAddDictionaryEntry(dictionary, ReadOnlySpan<uint32_t>(key2, 2), 20);

    // Assert
    auto value1 = SystemGetDictionaryValue(dictionary, ReadOnlySpan<uint32_t>(key1, 2));
    auto value2 = SystemGetDictionaryValue(dictionary, ReadOnlySpan<uint32_t>(key2, 2));
    ASSERT_TRUE(value1 != nullptr);
    ASSERT_TRUE(value2 != nullptr);
    ASSERT_EQ(10, *value1);
    ASSERT_EQ(20, *value2);
}

UTEST(Dictionary, MissingValueReturnsNull)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<int32_t, int32_t>(stackMemoryArena, 8);

    // Act
    auto value = SystemGetDictionaryValue(dictionary, 42);

    // Assert
    ASSERT_TRUE(value == nullptr);
    ASSERT_EQ(0, dictionary[42]);
}
