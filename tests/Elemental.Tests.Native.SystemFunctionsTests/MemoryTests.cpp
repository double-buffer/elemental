#include "SystemMemory.h"
#include "utest.h"

// TODO: Write thread safe tests!!!!

UTEST(Memory, Allocate) 
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataSizeInBytes = 1024llu;
    
    // Act
    auto data = SystemPushArrayZero<uint8_t>(memoryArena, dataSizeInBytes); 

    // Assert
    ASSERT_EQ(dataSizeInBytes, SystemGetMemoryArenaAllocatedBytes(memoryArena));
    ASSERT_EQ(dataSizeInBytes, data.Length);
}

UTEST(Memory, ConcatBuffers)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena(1024);
    
    // Act
    auto result = SystemConcatBuffers<char>(memoryArena, "Test1", "Test2");

    // Assert
    ASSERT_STREQ("Test1Test2", result.Pointer);
}

UTEST(Memory, StackMemoryArena)
{
    // Arrange
    auto stackMemoryArena1 = SystemGetStackMemoryArena();
    auto string1 = SystemConcatBuffers<char>(stackMemoryArena1, "Test", "Stack1");
    ReadOnlySpan<char> string2;
    ReadOnlySpan<char> string5;

    // Act
    {
        auto stackMemoryArena2 = SystemGetStackMemoryArena();

        string2 = SystemConcatBuffers<char>(stackMemoryArena1, "Test2", "Stack1");
        SystemPushMemory(stackMemoryArena1, 10000);
        SystemConcatBuffers<char>(stackMemoryArena2, "Test", "Stack2");

        {
            auto stackMemoryArena3 = SystemGetStackMemoryArena();
            string5 = SystemConcatBuffers<char>(stackMemoryArena1, "Test4", "Stack1");
        }
    }

    auto string4 = SystemConcatBuffers<char>(stackMemoryArena1, "Test3", "Stack1");

    // Assert
    ASSERT_STREQ("TestStack1", string1.Pointer);
    ASSERT_STREQ("Test2Stack1", string2.Pointer);
    ASSERT_STREQ("Test3Stack1", string4.Pointer);
    ASSERT_STREQ("Test4Stack1", string5.Pointer);
}

UTEST(Memory, StackMemoryArenaRelease)
{
    // Arrange
    auto stackMemoryArena1 = SystemGetStackMemoryArena();
    auto string1 = SystemConcatBuffers<char>(stackMemoryArena1, "Test", "Stack1");
    ReadOnlySpan<char> string2;
    ReadOnlySpan<char> string3;
    ReadOnlySpan<char> string4;
    ReadOnlySpan<char> string5;

    // Act
    {
        auto stackMemoryArena2 = SystemGetStackMemoryArena();
        string2 = SystemConcatBuffers<char>(stackMemoryArena1, "Test2", "Stack1");
        
        {
            auto stackMemoryArena3 = SystemGetStackMemoryArena();
            SystemConcatBuffers<char>(stackMemoryArena2, "Test", "Stack2");
        }

        {
            auto memoryArenaPointer = (MemoryArena)stackMemoryArena1;
            auto stackMemoryArena4 = SystemGetStackMemoryArena();
            {
                auto stackMemoryArena3 = SystemGetStackMemoryArena();
                SystemConcatBuffers<char>(stackMemoryArena3, "Test", "Stack2");
                SystemConcatBuffers<char>(stackMemoryArena3, "Test", "Stack2");
                SystemConcatBuffers<char>(stackMemoryArena3, "Test", "Stack2");
            }
            
            SystemConcatBuffers<char>(stackMemoryArena4, "Test", "Stack2");
            string5 = SystemConcatBuffers<char>(memoryArenaPointer, "Test5", "Stack1");
        }
            
        SystemConcatBuffers<char>(stackMemoryArena2, "Test2", "Stack2");
        string3 = SystemConcatBuffers<char>(stackMemoryArena1, "Test3", "Stack1");
    }

    {
        auto stackMemoryArena3 = SystemGetStackMemoryArena();
        string4 = SystemConcatBuffers<char>(stackMemoryArena1, "Test4", "Stack1");
    }

    // Assert
    ASSERT_STREQ("TestStack1", string1.Pointer);
    ASSERT_STREQ("Test2Stack1", string2.Pointer);
    ASSERT_STREQ("Test3Stack1", string3.Pointer);
    ASSERT_STREQ("Test4Stack1", string4.Pointer);
    ASSERT_STREQ("Test5Stack1", string5.Pointer);
}

UTEST(Memory, StackPushAllocateNewBlock) 
{
    // Arrange
    auto memoryArena = SystemGetStackMemoryArena();
    auto dataSizeInBytes = 2097152llu;
    auto data = SystemPushArrayZero<uint8_t>(memoryArena, dataSizeInBytes); 
    
    // Act
    SystemPushArrayZero<uint8_t>(memoryArena, 1024); 

    // Assert
    ASSERT_LT(dataSizeInBytes + 1024, SystemGetMemoryArenaAllocatedBytes(memoryArena));
    ASSERT_EQ(dataSizeInBytes, data.Length);
}
