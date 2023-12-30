#pragma once

#include "SystemSpan.h"

// TODO: Get rid of all mallocs and new

//#define new NOT_IMPLEMENTED()

//#define malloc(size) NOT_IMPLEMENTED(size)
//#define calloc(count, size) NOT_IMPLEMENTED(count, size)
//#define free(pointer) NOT_IMPLEMENTED(pointer)

//#define new NOT_IMPLEMENTED

/**
 * Represents the storage structure for MemoryArena.
 */
struct MemoryArenaStorage;

/**
 * Represents a memory arena that manages memory allocation.
 */
struct MemoryArena
{
    MemoryArenaStorage* Storage;    ///< Pointer to the storage structure.
    size_t StartOffset;             ///< Starting offset of the allocated memory.
    size_t AllocatedBytes;          ///< Total bytes currently allocated.
    size_t SizeInBytes;             ///< Total size of the memory arena.
    uint8_t Level;                  ///< Memory arena level.
    MemoryArena* ExtraStorage;      ///< Pointer to extra memory storage.
};

/**
 * Represents a stack-based memory arena.
 */
struct StackMemoryArena
{
    MemoryArena* MemoryArenaPointer; ///< Pointer to the associated MemoryArena.

    /**
     * Destructor for StackMemoryArena.
     */
    ~StackMemoryArena();

    /**
     * Conversion operator to MemoryArena pointer.
     * @return Pointer to the associated MemoryArena.
     */
    operator MemoryArena*() const 
    {
        return MemoryArenaPointer;
    }
};

/**
 * Allocates a new MemoryArena with a default size.
 * @return A pointer to the newly allocated MemoryArena.
 */
MemoryArena* SystemAllocateMemoryArena();

/**
 * Allocates a new MemoryArena with a specified size in bytes.
 * @param sizeInBytes The size, in bytes, to allocate for the MemoryArena.
 * @return A pointer to the newly allocated MemoryArena.
 */
MemoryArena* SystemAllocateMemoryArena(size_t sizeInBytes);

/**
 * Frees the memory associated with a MemoryArena.
 * @param memoryArena A pointer to the MemoryArena to be freed.
 */
void SystemFreeMemoryArena(MemoryArena* memoryArena);

/**
 * Clears the contents of a MemoryArena.
 * @param memoryArena A pointer to the MemoryArena to be cleared.
 */
void SystemClearMemoryArena(MemoryArena* memoryArena);

/**
 * Gets the number of bytes currently allocated in a MemoryArena.
 * @param memoryArena A pointer to the MemoryArena.
 * @return The number of bytes currently allocated in the MemoryArena.
 */
size_t SystemGetMemoryArenaAllocatedBytes(MemoryArena* memoryArena);

/**
 * Gets a StackMemoryArena, a specialized MemoryArena with stack-based allocation.
 * @return A StackMemoryArena.
 */
StackMemoryArena SystemGetStackMemoryArena();

/**
 * Allocates a block of memory in the specified MemoryArena.
 * @param memoryArena A pointer to the MemoryArena.
 * @param sizeInBytes The size, in bytes, to allocate.
 * @return A pointer to the allocated memory block.
 */
void* SystemPushMemory(MemoryArena* memoryArena, size_t sizeInBytes);

/**
 * Frees a block of memory in the specified MemoryArena.
 * @param memoryArena A pointer to the MemoryArena.
 * @param sizeInBytes The size, in bytes, to free.
 */
void SystemPopMemory(MemoryArena* memoryArena, size_t sizeInBytes);

/**
 * Allocates and initializes a block of memory with zero values in the specified MemoryArena.
 * @param memoryArena A pointer to the MemoryArena.
 * @param sizeInBytes The size, in bytes, to allocate and initialize.
 * @return A pointer to the allocated and initialized memory block.
 */
void* SystemPushMemoryZero(MemoryArena* memoryArena, size_t sizeInBytes);

/**
 * Allocates an array of elements in the specified MemoryArena.
 * @tparam T The type of elements in the array.
 * @param memoryArena A pointer to the MemoryArena.
 * @param count The number of elements to allocate.
 * @return A Span<T> representing the newly allocated array.
 */
template<typename T>
Span<T> SystemPushArray(MemoryArena* memoryArena, size_t count);

/**
 * Allocates and initializes an array of elements with zero values in the specified MemoryArena.
 * @tparam T The type of elements in the array.
 * @param memoryArena A pointer to the MemoryArena.
 * @param count The number of elements to allocate and initialize.
 * @return A Span<T> representing the newly allocated and initialized array.
 */
template<typename T>
Span<T> SystemPushArrayZero(MemoryArena* memoryArena, size_t count);

/**
 * Allocates and initializes an array of elements with zero values in the specified MemoryArena.
 * @param memoryArena A pointer to the MemoryArena.
 * @param count The number of elements to allocate and initialize.
 * @return A Span<char> representing the newly allocated and initialized array.
 */
template<>
Span<char> SystemPushArrayZero(MemoryArena* memoryArena, size_t count);

/**
 * Allocates and initializes an array of elements with zero values in the specified MemoryArena.
 * @param memoryArena A pointer to the MemoryArena.
 * @param count The number of elements to allocate and initialize.
 * @return A Span<wchar_t> representing the newly allocated and initialized array.
 */
template<>
Span<wchar_t> SystemPushArrayZero(MemoryArena* memoryArena, size_t count);

/**
 * Allocates a single instance of a structure in the specified MemoryArena.
 * @tparam T The type of structure to allocate.
 * @param memoryArena A pointer to the MemoryArena.
 * @return A pointer to the newly allocated structure.
 */
template<typename T>
T* SystemPushStruct(MemoryArena* memoryArena);

/**
 * Allocates and initializes a single instance of a structure with zero values in the specified MemoryArena.
 * @tparam T The type of structure to allocate.
 * @param memoryArena A pointer to the MemoryArena.
 * @return A pointer to the newly allocated and initialized structure.
 */
template<typename T>
T* SystemPushStructZero(MemoryArena* memoryArena);

/**
 * Copies elements from a source buffer to a destination buffer.
 * @tparam T The type of elements in the buffers.
 * @param destination A Span<T> representing the destination buffer.
 * @param source A ReadOnlySpan<T> representing the source buffer.
 */
template<typename T>
void SystemCopyBuffer(Span<T> destination, ReadOnlySpan<T> source);

/**
 * Concatenates two buffers into a new buffer allocated in the specified memory arena.
 * @tparam T The type of elements in the buffers.
 * @param memoryArena The memory arena to allocate space for the concatenated buffer.
 * @param buffer1 A ReadOnlySpan<T> representing the first buffer to concatenate.
 * @param buffer2 A ReadOnlySpan<T> representing the second buffer to concatenate.
 * @return A Span<T> representing the newly allocated concatenated buffer.
 */
template<typename T>
Span<T> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2);

/**
 * Concatenates two buffers into a new buffer allocated in the specified memory arena.
 * @param memoryArena The memory arena to allocate space for the concatenated buffer.
 * @param buffer1 A ReadOnlySpan<char> representing the first buffer to concatenate.
 * @param buffer2 A ReadOnlySpan<char> representing the second buffer to concatenate.
 * @return A Span<char> representing the newly allocated concatenated buffer.
 */
template<>
Span<char> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<char> buffer1, ReadOnlySpan<char> buffer2);

/**
 * Concatenates two buffers into a new buffer allocated in the specified memory arena.
 * @param memoryArena The memory arena to allocate space for the concatenated buffer.
 * @param buffer1 A ReadOnlySpan<wchar_t> representing the first buffer to concatenate.
 * @param buffer2 A ReadOnlySpan<wchar_t> representing the second buffer to concatenate.
 * @return A Span<wchar_t> representing the newly allocated concatenated buffer.
 */
template<>
Span<wchar_t> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<wchar_t> buffer1, ReadOnlySpan<wchar_t> buffer2);
