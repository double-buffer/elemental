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

    bool IsEmpty()
    {
        return Length == 0;
    }

    template<typename U>
    Span<U> Cast()
    {
        return Span<U>((U*)Pointer, Length);
    }

    void CopyTo(Span<T> destination)
    {
        // TODO: Add checks
        memcpy(destination.Pointer, Pointer, Length * sizeof(T));
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

    static Span<T> Empty()
    {
        return Span<T>();
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
        Length = strlen(stringValue);
    }

    ReadOnlySpan(const char* stringValue, size_t length)
    {
        Pointer = stringValue;
        Length = length;
    }
    
    ReadOnlySpan(const wchar_t* stringValue)
    {
        Pointer = stringValue;
        Length = wcslen(stringValue);
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

    bool IsEmpty()
    {
        return Length == 0;
    }

    template<typename U>
    ReadOnlySpan<U> Cast()
    {
        return ReadOnlySpan<U>((U*)Pointer, Length);
    }

    void CopyTo(Span<T> destination)
    {
        // TODO: Add checks
        memcpy(destination.Pointer, Pointer, Length * sizeof(T));
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

    static ReadOnlySpan<T> Empty()
    {
        return Span<T>();
    }
};

