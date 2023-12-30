#include "SystemDictionary.h"
#include "SystemFunctions.h"
#include "utest.h"

struct ThreadParameter
{
    SystemDictionary<int32_t, int32_t> Dictionary;
    int32_t ThreadId;
};

struct TestStruct
{
    int64_t Value1;
    int64_t Value2;
};

void ConcurrentAddFunction(void* parameter)
{
    auto threadParameter = (ThreadParameter*)parameter;
    auto dictionary = threadParameter->Dictionary;

    for (int32_t i = 0; i < 1000; i++)
    {
        SystemAddDictionaryEntry(dictionary, threadParameter->ThreadId * 1000 + i, i);
    }
}

void ConcurrentRemoveFunction(void* parameter)
{
    auto threadParameter = (ThreadParameter*)parameter;
    auto dictionary = threadParameter->Dictionary;

    for (int32_t i = 0; i < 1000; i++)
    {
        SystemRemoveDictionaryEntry(dictionary, threadParameter->ThreadId * 1000 + i);
    }
}

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
    auto dictionary = SystemCreateDictionary<int32_t, TestStruct>(stackMemoryArena, 24);
    
    // Act
    for (int32_t i = 0; i < 10; i++)
    {
        TestStruct testStruct = {};
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
    SystemDebugDictionary(dictionary);

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

// TODO: Add tests for ADD same key normal/concurrent
// TODO: Add tests for REMOVE same key normal/concurrent 

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
    SystemDebugDictionary(dictionary);

    // Assert
    auto testValue = dictionary["TestNew"];
    ASSERT_EQ(28, testValue);
}

UTEST(Dictionary, BigDictionary) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<char>, int32_t>(stackMemoryArena, 10000);
    
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

UTEST(Dictionary, ConcurrentAdd) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<int32_t, int32_t>(stackMemoryArena, 10000);
    
    // Act
    SystemThread threads[10];

    for (int32_t i = 0; i < 10; i++)
    {
        ThreadParameter parameter = { dictionary, i };
        threads[i] = SystemCreateThread(ConcurrentAddFunction, &parameter);
    }

    for (int32_t i = 0; i < 10; i++)
    {
        SystemWaitThread(threads[i]);
    }

    // Assert
    auto testEnumerator = SystemGetDictionaryEnumerator(dictionary);
    auto count = 0;
    auto testValue = SystemGetDictionaryEnumeratorNextValue(&testEnumerator);

    while (testValue != nullptr)
    {
        count++;
        testValue = SystemGetDictionaryEnumeratorNextValue(&testEnumerator);
    }

    ASSERT_EQ(10000, count);
}

UTEST(Dictionary, ConcurrentRemove) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<int32_t, int32_t>(stackMemoryArena, 10000);
    
    for (int32_t i = 0; i < 10000; i++)
    {
        SystemAddDictionaryEntry(dictionary, i, i);
    }
    
    // Act
    SystemThread threads[5];

    for (int32_t i = 0; i < 5; i++)
    {
        ThreadParameter parameter = { dictionary, i };
        threads[i] = SystemCreateThread(ConcurrentRemoveFunction, &parameter);
        SystemWaitThread(threads[i]);
    }

    for (int32_t i = 0; i < 5; i++)
    {
    }

    // Assert
    auto testEnumerator = SystemGetDictionaryEnumerator(dictionary);
    auto count = 0;
    auto testValue = SystemGetDictionaryEnumeratorNextValue(&testEnumerator);

    while (testValue != nullptr)
    {
        count++;
        testValue = SystemGetDictionaryEnumeratorNextValue(&testEnumerator);
    }

    ASSERT_EQ(5000, count);
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
    
    // Act
    for (int32_t i = 0; i < 10; i++)
    {
        SystemAddDictionaryEntry(dictionary, i, i);
    }

    // Act
    auto testValue = SystemDictionaryContainsKey(dictionary, 9);

    // Assert
    ASSERT_TRUE(testValue);
}

UTEST(Dictionary, Enumeration) 
{
    // Arrange
    auto maxEntries = 100;
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<int32_t, TestStruct>(stackMemoryArena, maxEntries);
    
    // Act
    for (int32_t i = 0; i < maxEntries / 2; i++)
    {
        TestStruct testStruct = {};
        testStruct.Value1 = i;
        testStruct.Value2 = i * i;

        SystemAddDictionaryEntry(dictionary, i, testStruct);
    }

    // Act
    auto testEnumerator = SystemGetDictionaryEnumerator(dictionary);
    auto count = 0;
    TestStruct resultItems[maxEntries];
    auto testValue = SystemGetDictionaryEnumeratorNextValue(&testEnumerator);

    while (testValue != nullptr)
    {
        resultItems[count++] = *testValue;
        testValue = SystemGetDictionaryEnumeratorNextValue(&testEnumerator);
    }

    // Assert
    ASSERT_EQ(maxEntries / 2, count);

    for (int32_t i = 0; i < maxEntries / 2; i++)
    {
        ASSERT_EQ(i, resultItems[i].Value1);
        ASSERT_EQ(i * i, resultItems[i].Value2);
    }
}
