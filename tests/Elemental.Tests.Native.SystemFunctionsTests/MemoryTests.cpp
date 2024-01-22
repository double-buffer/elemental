#include "SystemFunctions.h"
#include "SystemMemory.h"
#include "utest.h"

// TODO: Write memory alignment check tests

struct MemoryThreadParameter
{
    MemoryArena MemoryArena;
    int32_t ThreadId;
    int32_t ItemCount;
};

void MemoryConcurrentAddFunction(void* parameter)
{
    auto threadParameter = (MemoryThreadParameter*)parameter;

    for (int32_t i = 0; i < threadParameter->ItemCount; i++)
    {
        SystemPushMemory(threadParameter->MemoryArena, 64);
    }
}

void MemoryConcurrentPopFunction(void* parameter)
{
    auto threadParameter = (MemoryThreadParameter*)parameter;

    for (int32_t i = 0; i < threadParameter->ItemCount; i++)
    {
        SystemPopMemory(threadParameter->MemoryArena, 64);
    }
}

UTEST(Memory, Allocate) 
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataSizeInBytes = 70024llu;
    
    // Act
    auto data = SystemPushArrayZero<uint8_t>(memoryArena, dataSizeInBytes); 

    // Assert
    ASSERT_EQ(dataSizeInBytes, SystemGetMemoryArenaAllocatedBytes(memoryArena));
    ASSERT_EQ(dataSizeInBytes, data.Length);
}

UTEST(Memory, AllocateMultiple) 
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataSizeInBytes = 70024llu;
    auto dataSizeInBytes2 = 1024llu;
    
    // Act
    SystemPushArrayZero<uint8_t>(memoryArena, dataSizeInBytes); 
    SystemPushArrayZero<uint8_t>(memoryArena, dataSizeInBytes2); 
    SystemPopMemory(memoryArena, dataSizeInBytes2);

    // Assert
    ASSERT_EQ(dataSizeInBytes, SystemGetMemoryArenaAllocatedBytes(memoryArena));
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

UTEST(Memory, ConcurrentPush) 
{
    // Arrange
    const int32_t itemCount = 80000;
    const int32_t threadCount = 32;
    auto maxSize = (size_t)itemCount * 64;
    auto memoryArena = SystemAllocateMemoryArena(maxSize);
    
    // Act
    SystemThread threads[threadCount];
    MemoryThreadParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { memoryArena, i, itemCount / threadCount };
        threads[i] = SystemCreateThread(MemoryConcurrentAddFunction, &threadParameters[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    ASSERT_EQ(maxSize, SystemGetMemoryArenaAllocatedBytes(memoryArena));
}

UTEST(Memory, ConcurrentPop) 
{
    // Arrange
    const int32_t itemCount = 80000;
    const int32_t threadCount = 32;
    auto maxSize = (size_t)itemCount * 64;
    auto memoryArena = SystemAllocateMemoryArena(maxSize);
    SystemPushMemory(memoryArena, maxSize);
    
    // Act
    SystemThread threads[threadCount];
    MemoryThreadParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { memoryArena, i, itemCount / threadCount };
        threads[i] = SystemCreateThread(MemoryConcurrentPopFunction, &threadParameters[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    ASSERT_EQ(0llu, SystemGetMemoryArenaAllocatedBytes(memoryArena));
}

UTEST(Memory, ConcurrentPushPop) 
{
    // Arrange
    const int32_t itemCount = 80000;
    const int32_t threadCount = 32;
    auto maxSize = (size_t)itemCount * 64;
    auto memoryArena = SystemAllocateMemoryArena(maxSize);
    SystemPushMemory(memoryArena, maxSize / 2);
    
    // Act
    SystemThread threads[threadCount];
    MemoryThreadParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount / 2; i++)
    {
        threadParameters[i] = { memoryArena, i, itemCount / threadCount };
        threads[i] = SystemCreateThread(MemoryConcurrentPopFunction, &threadParameters[i]);
    }
    
    for (int32_t i = threadCount / 2; i < threadCount; i++)
    {
        threadParameters[i] = { memoryArena, i, itemCount / threadCount };
        threads[i] = SystemCreateThread(MemoryConcurrentAddFunction, &threadParameters[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    ASSERT_EQ(maxSize / 2, SystemGetMemoryArenaAllocatedBytes(memoryArena));
}

// TODO: Add tests with manual commit decommit over a reserved area


// TODO: ConcurrentPush and Pop mixed to tead the decommit thread safe that could have issues
