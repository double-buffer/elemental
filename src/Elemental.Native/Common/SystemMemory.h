#pragma once

#include "Dictionary.h"
#include "SystemLogging.h"

#ifdef _DEBUG

typedef struct
{
    size_t SizeInBytes;
    wchar_t File[MAX_PATH];
    uint32_t LineNumber;
} SystemAllocation;

void* SystemAllocateMemory(size_t sizeInBytes, const wchar_t* file, uint32_t lineNumber);
void* SystemAllocateMemoryAndReset(size_t count, size_t size, const wchar_t* file, uint32_t lineNumber);
void SystemFreeMemory(void* pointer);
void SystemDisplayMemoryLeak(uint64_t key, void* data);
void SystemInitDebugAllocations();
void SystemCheckAllocations(const wchar_t* description);

void* operator new(size_t size, const wchar_t* file, uint32_t lineNumber);
void* operator new[](size_t size, const wchar_t* file, uint32_t lineNumber);
void operator delete(void* pointer) noexcept;
void operator delete[](void* pointer) noexcept;
void operator delete(void* pointer, const wchar_t* file, uint32_t lineNumber);
void operator delete[](void* pointer, const wchar_t* file, uint32_t lineNumber);

//#define malloc(size) NOT_IMPLEMENTED(size)

#endif

// New CODE
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


struct MemoryArena
{
    Span<uint8_t> Memory;
    size_t AllocatedBytes;
};

MemoryArena* SystemAllocateMemoryArena(size_t sizeInBytes);

template<typename T>
Span<T> SystemPushArray(MemoryArena* memoryArena, size_t count); 

template<typename T>
Span<T> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2);
