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
    ASSERT_EQ_MSG(2llu, slice.Length, "ReadOnlySpan slice length is invalid.");
    ASSERT_EQ_MSG(20, slice[0], "ReadOnlySpan slice first value is invalid.");
    ASSERT_EQ_MSG(30, slice[1], "ReadOnlySpan slice second value is invalid.");
}

UTEST(Span, InitializerList)
{
    // Act
    auto result = SumSpanValues({ 10, 20, 30 });

    // Assert
    ASSERT_EQ_MSG(60, result, "ReadOnlySpan initializer-list values were not preserved.");
}

UTEST(Span, StringLengthExcludesNullTerminator)
{
    // Arrange
    ReadOnlySpan<char> value = "Elemental";

    // Assert
    ASSERT_EQ_MSG(9llu, value.Length, "Character span length should exclude the null terminator.");
    ASSERT_EQ_MSG('\0', value.Pointer[value.Length], "Character span backing storage should remain null terminated.");
}

UTEST(Span, DuplicateStringPreservesLogicalLengthAndNullTerminator)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena(1024);
    ReadOnlySpan<char> source = "Elemental";

    // Act
    auto result = SystemDuplicateBuffer<char>(memoryArena, source);

    // Assert
    ASSERT_EQ_MSG(source.Length, result.Length, "Duplicated character span should preserve the logical source length.");
    ASSERT_EQ_MSG('\0', result.Pointer[result.Length], "Duplicated character span should have a trailing null terminator.");
    ASSERT_STREQ_MSG("Elemental", result.Pointer, "Duplicated character span data is invalid.");

    SystemFreeMemoryArena(memoryArena);
}

UTEST(Span, DuplicateWideStringPreservesLogicalLengthAndNullTerminator)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena(1024);
    ReadOnlySpan<wchar_t> source = L"Elemental";

    // Act
    auto result = SystemDuplicateBuffer<wchar_t>(memoryArena, source);

    // Assert
    ASSERT_EQ_MSG(source.Length, result.Length, "Duplicated wide-character span should preserve the logical source length.");
    ASSERT_EQ_MSG(L'\0', result.Pointer[result.Length], "Duplicated wide-character span should have a trailing null terminator.");

    for (size_t i = 0; i < source.Length; i++)
    {
        ASSERT_EQ_MSG(source[i], result[i], "Duplicated wide-character span data is invalid.");
    }

    SystemFreeMemoryArena(memoryArena);
}
