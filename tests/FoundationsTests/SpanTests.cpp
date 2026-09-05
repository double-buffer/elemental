#include "SystemMemory.h"
#include "SystemSpan.h"
#include "utest.h"

int32_t SumSpanValues(ReadOnlySpan<int32_t> values)
{
    auto result = 0;

    for (size_t i = 0; i < values.Length; i++)
    {
        result += values[i];
    }

    return result;
}

UTEST(Span, ReadOnlyConstBuffer)
{
    // Arrange
    const int32_t values[] = { 10, 20, 30, 40 };
    const ReadOnlySpan<int32_t> span(values, 4);

    // Act
    auto slice = span.Slice(1, 2);

    // Assert
    ASSERT_EQ(2llu, slice.Length);
    ASSERT_EQ(20, slice[0]);
    ASSERT_EQ(30, slice[1]);
}

UTEST(Span, InitializerList)
{
    // Act
    auto result = SumSpanValues({ 10, 20, 30 });

    // Assert
    ASSERT_EQ(60, result);
}

UTEST(Span, StringLengthExcludesNullTerminator)
{
    // Arrange
    ReadOnlySpan<char> value = "Elemental";

    // Assert
    ASSERT_EQ(9llu, value.Length);
    ASSERT_EQ('\0', value.Pointer[value.Length]);
}

UTEST(Span, DuplicateStringPreservesLogicalLengthAndNullTerminator)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena(1024);
    ReadOnlySpan<char> source = "Elemental";

    // Act
    auto result = SystemDuplicateBuffer<char>(memoryArena, source);

    // Assert
    ASSERT_EQ(source.Length, result.Length);
    ASSERT_EQ('\0', result.Pointer[result.Length]);
    ASSERT_STREQ("Elemental", result.Pointer);

    SystemFreeMemoryArena(memoryArena);
}
