#pragma once

#include "SystemSpan.h"

// TODO: Get rid of all mallocs and new
// TODO: Maybe we should expose the memory arena to the outside world?

//#define new NOT_IMPLEMENTED()

//#define malloc(size) NOT_IMPLEMENTED(size)
//#define calloc(count, size) NOT_IMPLEMENTED(count, size)
//#define free(pointer) NOT_IMPLEMENTED(pointer)

//#define new NOT_IMPLEMENTED


/**
 * Enumeration representing the allocation state of a memory block.
 */
enum AllocationState
{
    AllocationState_Committed, ///< Indicates the memory block is allocated and committed.
    AllocationState_Reserved   ///< Indicates the memory block is reserved but not committed.
};

/**
 * Struct providing information about overall memory allocation.
 */
struct AllocationInfos
{
    size_t CommittedBytes; ///< Total bytes of committed memory.
    size_t ReservedBytes;  ///< Total bytes of reserved memory.
};

struct MemoryArenaStorage;

/**
 * Represents a memory arena for managing memory allocations efficiently.
 */
struct MemoryArena
{
    MemoryArenaStorage* Storage; ///< Internal storage structure of the memory arena.
    uint8_t Level;               ///< Nesting level of the memory arena.
};

/**
 * Struct providing detailed information about allocations within a MemoryArena.
 */
struct MemoryArenaAllocationInfos
{
    size_t AllocatedBytes;     ///< Total bytes currently allocated in the MemoryArena.
    size_t CommittedBytes;     ///< Total bytes committed in the MemoryArena.
    size_t MaximumSizeInBytes; ///< Maximum allocatable size of the MemoryArena in bytes.
};

/**
 * Specialized MemoryArena for stack-based memory management.
 */
struct StackMemoryArena
{
    MemoryArena Arena; ///< Associated MemoryArena object.

    size_t StartOffsetInBytes;     ///< Starting offset for memory allocations within the arena.
    size_t StartExtraOffsetInBytes; ///< Additional internal offset.

    /**
     * Destructor for StackMemoryArena.
     */
    ~StackMemoryArena();

    /**
     * Conversion operator to a MemoryArena pointer.
     * @return Pointer to the associated MemoryArena.
     */
    operator MemoryArena() const
    {
        return Arena;
    }
};

/**
 * Retrieves information about system-wide memory allocations.
 * @return AllocationInfos structure with memory allocation details.
 */
AllocationInfos SystemGetAllocationInfos();

/**
 * Creates a new MemoryArena with a default size.
 * Default size is 64GB. Note that the memory is reserved but committed only when needed.
 * @return Pointer to the newly created MemoryArena.
 */
MemoryArena SystemAllocateMemoryArena();

/**
 * Creates a new MemoryArena with a specified size.
 * Note that the memory is reserved but committed only when needed.
 * @param sizeInBytes Size of the MemoryArena in bytes.
 * @return Pointer to the newly created MemoryArena.
 */
MemoryArena SystemAllocateMemoryArena(size_t sizeInBytes);

/**
 * Frees the memory associated with a MemoryArena.
 * @param memoryArena Pointer to the MemoryArena to be freed.
 */
void SystemFreeMemoryArena(MemoryArena memoryArena);

/**
 * Clears the contents of a MemoryArena.
 * @param memoryArena Pointer to the MemoryArena to be cleared.
 */
void SystemClearMemoryArena(MemoryArena memoryArena);

/**
 * Retrieves allocation information for a specific MemoryArena.
 * @param memoryArena MemoryArena to query.
 * @return MemoryArenaAllocationInfos structure with detailed allocation information.
 */
MemoryArenaAllocationInfos SystemGetMemoryArenaAllocationInfos(MemoryArena memoryArena);

/**
 * Gets a StackMemoryArena, a specialized MemoryArena with stack-based allocation.
 * @return A StackMemoryArena.
 */
StackMemoryArena SystemGetStackMemoryArena();

/**
 * Allocates a block of memory in a MemoryArena.
 * @param memoryArena MemoryArena for the allocation.
 * @param sizeInBytes Size of the memory block to allocate.
 * @param state Allocation state (committed or reserved).
 * @return Pointer to the allocated memory block.
 */
void* SystemPushMemory(MemoryArena memoryArena, size_t sizeInBytes, AllocationState state = AllocationState_Committed);

/**
 * Frees a block of memory in a MemoryArena.
 * @param memoryArena MemoryArena containing the block.
 * @param sizeInBytes Size of the memory block to free.
 */
void SystemPopMemory(MemoryArena memoryArena, size_t sizeInBytes);

/**
 * Commits a block of memory in a MemoryArena.
 * 
 * @param memoryArena The MemoryArena to commit memory in.
 * @param pointer     Start pointer for memory commitment.
 * @param sizeInBytes Size of memory block in bytes.
 * @param clearMemory If true, initializes memory to 0. Defaults to false.
 */
void SystemCommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes, bool clearMemory = false);

/**
 * Commits memory for an array of elements in a MemoryArena.
 * 
 * @tparam T          Element type in the buffer.
 * @param memoryArena The MemoryArena to commit memory in.
 * @param buffer      ReadOnlySpan representing an array of elements.
 * @param clearMemory If true, initializes memory to 0. Defaults to false.
 */
template<typename T>
void SystemCommitMemory(MemoryArena memoryArena, ReadOnlySpan<T> buffer, bool clearMemory = false);


/**
 * Decomits memory in the specified MemoryArena.
 * The page will be decommitted if all the allocations (including spaces between them) have been decommitted.
 * @param memoryArena MemoryArena in which to decommit memory.
 * @param pointer Pointer to start decommitting memory.
 * @param sizeInBytes Size of the memory block to decommit.
 */
void SystemDecommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes);

/**
 * Allocates and initializes a block of memory with zero values in the specified MemoryArena.
 * @param memoryArena A pointer to the MemoryArena.
 * @param sizeInBytes The size, in bytes, to allocate and initialize.
 * @return A pointer to the allocated and initialized memory block.
 */
void* SystemPushMemoryZero(MemoryArena memoryArena, size_t sizeInBytes);

/**
 * Allocates an array of elements in the specified MemoryArena.
 * @tparam T The type of elements in the array.
 * @param memoryArena A pointer to the MemoryArena.
 * @param count The number of elements to allocate.
 * @return A Span<T> representing the newly allocated array.
 */
template<typename T>
Span<T> SystemPushArray(MemoryArena memoryArena, size_t count, AllocationState state = AllocationState_Committed);

/**
 * Allocates and initializes an array of elements with zero values in the specified MemoryArena.
 * @tparam T The type of elements in the array.
 * @param memoryArena A pointer to the MemoryArena.
 * @param count The number of elements to allocate and initialize.
 * @return A Span<T> representing the newly allocated and initialized array.
 */
template<typename T>
Span<T> SystemPushArrayZero(MemoryArena memoryArena, size_t count);

/**
 * Allocates and initializes an array of elements with zero values in the specified MemoryArena.
 * @param memoryArena A pointer to the MemoryArena.
 * @param count The number of elements to allocate and initialize.
 * @return A Span<char> representing the newly allocated and initialized array.
 */
template<>
Span<char> SystemPushArrayZero(MemoryArena memoryArena, size_t count);

/**
 * Allocates and initializes an array of elements with zero values in the specified MemoryArena.
 * @param memoryArena A pointer to the MemoryArena.
 * @param count The number of elements to allocate and initialize.
 * @return A Span<wchar_t> representing the newly allocated and initialized array.
 */
template<>
Span<wchar_t> SystemPushArrayZero(MemoryArena memoryArena, size_t count);

/**
 * Allocates a single instance of a structure in the specified MemoryArena.
 * @tparam T The type of structure to allocate.
 * @param memoryArena A pointer to the MemoryArena.
 * @return A pointer to the newly allocated structure.
 */
template<typename T>
T* SystemPushStruct(MemoryArena memoryArena);

/**
 * Allocates and initializes a single instance of a structure with zero values in the specified MemoryArena.
 * @tparam T The type of structure to allocate.
 * @param memoryArena A pointer to the MemoryArena.
 * @return A pointer to the newly allocated and initialized structure.
 */
template<typename T>
T* SystemPushStructZero(MemoryArena memoryArena);

/**
 * Copies elements from a source buffer to a destination buffer.
 * @tparam T The type of elements in the buffers.
 * @param destination A Span<T> representing the destination buffer.
 * @param source A ReadOnlySpan<T> representing the source buffer.
 */
template<typename T>
void SystemCopyBuffer(Span<T> destination, ReadOnlySpan<T> source);

/**
 * Dupliquate elements from a source buffer to a destination buffer.
 * @tparam T The type of elements in the buffers.
 * @param memoryArena A pointer to the MemoryArena.
 * @param source A ReadOnlySpan<T> representing the source buffer.
 * @return A pointer to the newly allocated structure that contains a copy of source.
 */
template<typename T>
Span<T> SystemDuplicateBuffer(MemoryArena memoryArena, ReadOnlySpan<T> source);

/**
 * Dupliquate elements from a source buffer to a destination buffer.
 * @tparam T The type of elements in the buffers.
 * @param memoryArena A pointer to the MemoryArena.
 * @param source A ReadOnlySpan<T> representing the source buffer.
 * @return A pointer to the newly allocated structure that contains a copy of source.
 */
template<>
Span<char> SystemDuplicateBuffer(MemoryArena memoryArena, ReadOnlySpan<char> source);

/**
 * Concatenates two buffers into a new buffer allocated in the specified memory arena.
 * @tparam T The type of elements in the buffers.
 * @param memoryArena The memory arena to allocate space for the concatenated buffer.
 * @param buffer1 A ReadOnlySpan<T> representing the first buffer to concatenate.
 * @param buffer2 A ReadOnlySpan<T> representing the second buffer to concatenate.
 * @return A Span<T> representing the newly allocated concatenated buffer.
 */
template<typename T>
Span<T> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2);

/**
 * Concatenates two buffers into a new buffer allocated in the specified memory arena.
 * @param memoryArena The memory arena to allocate space for the concatenated buffer.
 * @param buffer1 A ReadOnlySpan<char> representing the first buffer to concatenate.
 * @param buffer2 A ReadOnlySpan<char> representing the second buffer to concatenate.
 * @return A Span<char> representing the newly allocated concatenated buffer.
 */
template<>
Span<char> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<char> buffer1, ReadOnlySpan<char> buffer2);

/**
 * Concatenates two buffers into a new buffer allocated in the specified memory arena.
 * @param memoryArena The memory arena to allocate space for the concatenated buffer.
 * @param buffer1 A ReadOnlySpan<wchar_t> representing the first buffer to concatenate.
 * @param buffer2 A ReadOnlySpan<wchar_t> representing the second buffer to concatenate.
 * @return A Span<wchar_t> representing the newly allocated concatenated buffer.
 */
template<>
Span<wchar_t> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<wchar_t> buffer1, ReadOnlySpan<wchar_t> buffer2);
