#pragma once

/**
 * Represents a span of elements in memory.
 *
 * The Span class provides a non-owning view over a contiguous sequence of elements.
 * It is designed for efficient and safe manipulation of arrays and memory buffers.
 *
 * @tparam T The type of elements in the span.
 */
template<typename T>
struct Span
{
    /**
     * Default constructor for an empty span.
     */
    Span()
    {
        Pointer = nullptr;
        Length = 0;
    }

    /**
     * Constructs a span with the given pointer and length.
     *
     * @param pointer The pointer to the first element of the span.
     * @param length The length of the span.
     */
    Span(T* pointer, size_t length)
    {
        Pointer = pointer;
        Length = length;
    }

    /**
     * Pointer to the first element of the span.
     */
    T* Pointer;

    /**
     * Length of the span.
     */
    size_t Length;

    /**
     * Accesses the element at the specified index.
     *
     * @param index The index of the element to access.
     * @return A reference to the element at the specified index.
     */
    T& operator[](int index)
    {
        // TODO: Check bounds
        return Pointer[index];
    }

    /**
     * Creates a new span that represents a slice of the current span.
     *
     * @param start The starting index of the slice.
     * @return A new span representing the sliced portion.
     */
    Span<T> Slice(size_t start)
    {
        // TODO: Add checks
        return Span<T>(Pointer + start, Length - start);
    }

    /**
     * Creates a new span that represents a sub-span of the current span.
     *
     * @param start The starting index of the sub-span.
     * @param length The length of the sub-span.
     * @return A new span representing the sub-span.
     */
    Span<T> Slice(size_t start, size_t length)
    {
        // TODO: Add checks
        return Span<T>(Pointer + start, length);
    }
};


/**
 * Represents a read-only span of elements in memory.
 *
 * The ReadOnlySpan class provides a non-owning, read-only view over a contiguous sequence of elements.
 *
 * @tparam T The type of elements in the read-only span.
 */
template<typename T>
struct ReadOnlySpan
{
    /**
     * Default constructor for an empty read-only span.
     */
    ReadOnlySpan()
    {
        Pointer = nullptr;
        Length = 0;
    }

    /**
     * Constructs a read-only span with the given pointer and length.
     *
     * @param pointer The pointer to the first element of the read-only span.
     * @param length The length of the read-only span.
     */
    ReadOnlySpan(T* pointer, size_t length)
    {
        Pointer = pointer;
        Length = length;
    }

    /**
     * Constructs a read-only span from a null-terminated string.
     *
     * @param stringValue A pointer to the null-terminated string.
     */
    ReadOnlySpan(const char* stringValue)
    {
        Pointer = stringValue;
        Length = 0;

        while (stringValue[Length] != '\0')
        {
            Length++;
        }
    }

    /**
     * Constructs a read-only span from a substring of a null-terminated string.
     *
     * @param stringValue A pointer to the null-terminated string.
     * @param length The length of the substring.
     */
    ReadOnlySpan(const char* stringValue, size_t length)
    {
        Pointer = stringValue;
        Length = length;
    }
    
    /**
     * Constructs a read-only span from a null-terminated wide string.
     *
     * @param stringValue A pointer to the null-terminated wide string.
     */
    ReadOnlySpan(const wchar_t* stringValue)
    {
        Pointer = stringValue;
        Length = 0;

        while (stringValue[Length] != '\0')
        {
            Length++;
        }
    }

    /**
     * Constructs a read-only span from a substring of a null-terminated wide string.
     *
     * @param stringValue A pointer to the null-terminated wide string.
     * @param length The length of the substring.
     */
    ReadOnlySpan(const wchar_t* stringValue, size_t length)
    {
        Pointer = stringValue;
        Length = length;
    }

    /**
     * Constructs a read-only span from a mutable span.
     *
     * @param spanValue A mutable span.
     */
    ReadOnlySpan(Span<T> spanValue)
    {
        Pointer = spanValue.Pointer;
        Length = spanValue.Length;
    }

    /**
     * Pointer to the first element of the read-only span.
     */
    const T* Pointer;

    /**
     * Length of the read-only span.
     */
    size_t Length;

    /**
     * Accesses the element at the specified index in a read-only manner.
     *
     * @param index The index of the element to access.
     * @return A const reference to the element at the specified index.
     */
    const T& operator[](int index)
    {
        // TODO: Check bounds
        return Pointer[index];
    }

    /**
     * Creates a new read-only span that represents a slice of the current span.
     *
     * @param start The starting index of the slice.
     * @return A new read-only span representing the sliced portion.
     */
    ReadOnlySpan<T> Slice(size_t start)
    {
        // TODO: Add checks
        return ReadOnlySpan<T>(Pointer + start, Length - start);
    }

    /**
     * Creates a new read-only span that represents a sub-span of the current span.
     *
     * @param start The starting index of the sub-span.
     * @param length The length of the sub-span.
     * @return A new read-only span representing the sub-span.
     */
    ReadOnlySpan<T> Slice(size_t start, size_t length)
    {
        // TODO: Add checks
        return ReadOnlySpan<T>(Pointer + start, length);
    }
};

