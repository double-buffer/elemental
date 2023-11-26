#pragma once

template<typename T>
struct Span
{
    Span()
    {
        Pointer = nullptr;
        Length = 0;
    }

    Span(T* pointer, size_t length)
    {
        Pointer = pointer;
        Length = length;
    }

    T* Pointer;
    size_t Length;

    T& operator[](int index)
    {
        // TODO: Check bounds
        return Pointer[index];
    }

    Span<T> Slice(size_t start)
    {
        // TODO: Add checks
        return Span<T>(Pointer + start, Length - start);
    }

    Span<T> Slice(size_t start, size_t length)
    {
        // TODO: Add checks
        return Span<T>(Pointer + start, length);
    }
};

template<typename T>
struct ReadOnlySpan
{
    ReadOnlySpan()
    {
        Pointer = nullptr;
        Length = 0;
    }

    ReadOnlySpan(T* pointer, size_t length)
    {
        Pointer = pointer;
        Length = length;
    }

    ReadOnlySpan(const char* stringValue)
    {
        Pointer = stringValue;
        Length = 0;

        while (stringValue[Length++] != '\0');
    }

    ReadOnlySpan(const char* stringValue, size_t length)
    {
        Pointer = stringValue;
        Length = length;
    }
    
    ReadOnlySpan(const wchar_t* stringValue)
    {
        Pointer = stringValue;
        Length = 0;

        while (stringValue[Length++] != '\0');
    }

    ReadOnlySpan(const wchar_t* stringValue, size_t length)
    {
        Pointer = stringValue;
        Length = length;
    }

    ReadOnlySpan(Span<T> spanValue)
    {
        Pointer = spanValue.Pointer;
        Length = spanValue.Length;
    }

    const T* Pointer;
    size_t Length;

    const T& operator[](int index)
    {
        // TODO: Check bounds
        return Pointer[index];
    }

    ReadOnlySpan<T> Slice(size_t start)
    {
        // TODO: Add checks
        return ReadOnlySpan<T>(Pointer + start, Length - start);
    }

    ReadOnlySpan<T> Slice(size_t start, size_t length)
    {
        // TODO: Add checks
        return ReadOnlySpan<T>(Pointer + start, length);
    }
};

