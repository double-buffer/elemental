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
 * Defines whether an arena allocation is immediately backed by committed memory.
 */
enum AllocationState
{
    AllocationState_Committed, ///< The allocated range is committed and can be accessed immediately.
    AllocationState_Reserved   ///< The allocated range is reserved only and must be committed before it is accessed.
};

/**
 * Provides process-wide virtual memory allocation information reported by the platform layer.
 */
struct AllocationInfos
{
    size_t CommittedBytes; ///< Total number of committed bytes.
    size_t ReservedBytes;  ///< Total number of reserved virtual-address bytes.
};

struct MemoryArenaStorage;

/**
 * Lightweight handle to a MemoryArena storage.
 *
 * MemoryArena is intentionally passed and copied by value. Copying a MemoryArena does not copy
 * its allocations or storage; every copy references the same MemoryArenaStorage. No ownership,
 * reference counting, or lifetime tracking is added by the handle.
 *
 * For regular arenas, Level is 0. For handles produced by StackMemoryArena, Level identifies the
 * stack lifetime associated with that handle and allows an ancestor arena to be passed down the
 * call tree while preserving the ancestor allocation lifetime.
 *
 * The caller is responsible for respecting the lifetime of the referenced storage. Freeing an
 * arena invalidates every MemoryArena value and every allocation that references that storage.
 */
struct MemoryArena
{
    MemoryArenaStorage* Storage; ///< Shared internal storage referenced by this handle.
    uint8_t Level;               ///< Stack lifetime level, or 0 for a regular arena.
};

/**
 * Provides allocation information for a MemoryArena.
 */
struct MemoryArenaAllocationInfos
{
    size_t AllocatedBytes;     ///< Bytes currently allocated from the arena data region.
    size_t CommittedBytes;     ///< Bytes currently committed by the arena, including its internal header pages.
    size_t MaximumSizeInBytes; ///< Maximum number of data bytes that can be allocated from the arena.
};

/**
 * Scoped thread-local MemoryArena.
 *
 * Destroying the StackMemoryArena releases allocations associated with its stack lifetime. The
 * contained MemoryArena can be passed by value to deeper functions while this scope is alive.
 * Passing a MemoryArena from an ancestor scope allows a deeper function to allocate data that
 * survives its local stack scopes and is released with that ancestor.
 *
 * StackMemoryArena is thread-local and must not be shared across threads. A MemoryArena obtained
 * from it must not be retained after the corresponding StackMemoryArena scope has ended.
 *
 * StackMemoryArena itself represents a scope and must not be copied by user code. Copying the
 * contained MemoryArena handle is the intended way to pass an allocation lifetime around.
 */
struct StackMemoryArena
{
    MemoryArena Arena; ///< MemoryArena handle associated with this stack scope.

    size_t StartOffsetInBytes;      ///< Internal data offset captured when the scope begins.
    size_t StartExtraOffsetInBytes; ///< Internal ancestor-lifetime storage offset captured when the scope begins.

    /**
     * Releases allocations owned by this stack scope and restores the previous stack lifetime.
     */
    ~StackMemoryArena();

    /**
     * Returns the lightweight MemoryArena handle associated with this stack scope.
     *
     * @return MemoryArena value that can be passed to allocation functions while this scope is alive.
     */
    operator MemoryArena() const
    {
        return Arena;
    }
};

/**
 * Retrieves process-wide virtual memory allocation information from the platform layer.
 *
 * @return Allocation information containing committed and reserved byte counts.
 */
AllocationInfos SystemGetAllocationInfos();

/**
 * Allocates a MemoryArena using the default capacity.
 *
 * The arena reserves its virtual address range up front while data pages are committed on demand.
 * The returned MemoryArena is a lightweight value handle to the allocated storage. The caller is
 * responsible for releasing that storage exactly once with SystemFreeMemoryArena().
 *
 * @return MemoryArena handle referencing the newly allocated storage.
 */
MemoryArena SystemAllocateMemoryArena();

/**
 * Allocates a MemoryArena with the specified data capacity.
 *
 * The arena reserves enough virtual address space for its internal metadata and requested data
 * capacity. Internal header pages are committed immediately; data pages are committed on demand.
 * The returned MemoryArena can be copied freely, but all copies reference the same storage.
 *
 * @param sizeInBytes Maximum number of data bytes that can be allocated from the arena.
 * @return MemoryArena handle referencing the newly allocated storage.
 */
MemoryArena SystemAllocateMemoryArena(size_t sizeInBytes);

/**
 * Releases the storage referenced by a MemoryArena.
 *
 * This is an exclusive lifetime operation and is not safe to call while another thread is using
 * the arena. All MemoryArena copies and all pointers/spans allocated from the arena become invalid
 * immediately after this call. The function does not perform reference counting or alias tracking.
 *
 * StackMemoryArena storage is managed by the stack arena system and must not be released through
 * this function.
 *
 * @param memoryArena MemoryArena whose storage will be released.
 */
void SystemFreeMemoryArena(MemoryArena memoryArena);

/**
 * Resets a MemoryArena to its initial empty state.
 *
 * All allocations made from the arena become invalid. The MemoryArena storage and copied handles
 * remain valid and can be used for new allocations after the reset.
 *
 * This is an exclusive operation and is intentionally not thread-safe. The caller must guarantee
 * that no other thread is reading from, allocating from, committing, or decommitting the arena.
 *
 * @param memoryArena MemoryArena to reset.
 */
void SystemClearMemoryArena(MemoryArena memoryArena);

/**
 * Retrieves allocation information for a MemoryArena.
 *
 * @param memoryArena MemoryArena to query.
 * @return Current allocated, committed, and maximum data-capacity information.
 */
MemoryArenaAllocationInfos SystemGetMemoryArenaAllocationInfos(MemoryArena memoryArena);

/**
 * Begins a new scoped MemoryArena lifetime on the current thread.
 *
 * Stack arenas are nested per thread. The returned object owns the scope rollback, while its
 * contained MemoryArena is the lightweight value intended to be passed down the call tree.
 * Allocating through an ancestor MemoryArena from a deeper scope preserves the ancestor lifetime.
 *
 * @return StackMemoryArena representing the newly entered stack scope.
 */
StackMemoryArena SystemGetStackMemoryArena();

/**
 * Allocates a contiguous range of bytes from a MemoryArena.
 *
 * The allocation advances the arena and is not individually freed. Regular shared MemoryArena
 * allocation is intended to be thread-safe; StackMemoryArena allocation is thread-local.
 *
 * A committed allocation can be accessed immediately. A reserved allocation only reserves its
 * range in the arena and must be committed with SystemCommitMemory() before access.
 *
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param sizeInBytes Number of bytes to allocate.
 * @param state Initial allocation state.
 * @return Pointer to the allocated range, or nullptr if the arena cannot satisfy the allocation.
 */
void* SystemPushMemory(MemoryArena memoryArena, size_t sizeInBytes, AllocationState state = AllocationState_Committed);

/**
 * Commits the pages covering a previously allocated range in a MemoryArena.
 *
 * The range must belong to the specified arena. Commitment is tracked at platform page granularity,
 * so pages shared by multiple logical ranges remain committed while any tracked range still needs
 * them. The operation is intended to be thread-safe for regular shared MemoryArena instances.
 *
 * If clearMemory is true, pages that are newly committed by this operation are cleared before use.
 * Use SystemPushMemoryZero() when the exact returned allocation range must be initialized to zero.
 *
 * @param memoryArena MemoryArena containing the range.
 * @param pointer Start of the range to commit.
 * @param sizeInBytes Number of bytes in the range.
 * @param clearMemory Whether newly committed pages should be cleared.
 */
void SystemCommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes, bool clearMemory = false);

/**
 * Commits the pages covering a previously allocated buffer in a MemoryArena.
 *
 * The buffer must reference memory allocated from the specified arena. Commitment is tracked at
 * platform page granularity.
 *
 * @tparam T Element type stored in the buffer.
 * @param memoryArena MemoryArena containing the buffer.
 * @param buffer Buffer whose memory range will be committed.
 * @param clearMemory Whether newly committed pages should be cleared.
 */
template<typename T>
void SystemCommitMemory(MemoryArena memoryArena, ReadOnlySpan<T> buffer, bool clearMemory = false);


/**
 * Decommits pages that are no longer needed by a range in a MemoryArena.
 *
 * Decommitting memory does not release the logical arena allocation or move the arena pointer. The
 * same reserved range can be committed again later. Physical pages are only decommitted when the
 * arena bookkeeping determines that no remaining committed range still needs that page.
 *
 * The caller is responsible for passing a valid range belonging to the arena and for not accessing
 * the range while it is decommitted. The operation is intended to be thread-safe for regular shared
 * MemoryArena instances.
 *
 * @param memoryArena MemoryArena containing the range.
 * @param pointer Start of the range to decommit.
 * @param sizeInBytes Number of bytes in the range.
 */
void SystemDecommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes);

/**
 * Allocates a committed range of bytes and initializes the requested range to zero.
 *
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param sizeInBytes Number of bytes to allocate and clear.
 * @return Pointer to the allocated range, or nullptr if the arena cannot satisfy the allocation.
 */
void* SystemPushMemoryZero(MemoryArena memoryArena, size_t sizeInBytes);

/**
 * Allocates a contiguous array from a MemoryArena.
 *
 * The returned Span references arena-owned memory and remains valid only for the lifetime of the
 * corresponding arena allocation context.
 *
 * @tparam T Element type to allocate.
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param count Number of elements to allocate.
 * @param state Initial allocation state.
 * @return Span referencing the allocated array.
 */
template<typename T>
Span<T> SystemPushArray(MemoryArena memoryArena, size_t count, AllocationState state = AllocationState_Committed);

/**
 * Allocates a contiguous array and initializes it to zero.
 *
 * @tparam T Element type to allocate.
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param count Number of elements to allocate and clear.
 * @return Span referencing the zero-initialized array.
 */
template<typename T>
Span<T> SystemPushArrayZero(MemoryArena memoryArena, size_t count);

/**
 * Allocates a zero-initialized char array with an additional zero terminator after the returned Span.
 *
 * The terminator is allocated immediately after the requested elements and is not included in the
 * returned Span length.
 *
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param count Number of char elements in the returned Span.
 * @return Span referencing the requested zero-initialized char elements.
 */
template<>
Span<char> SystemPushArrayZero(MemoryArena memoryArena, size_t count);

/**
 * Allocates a zero-initialized wchar_t array with an additional zero terminator after the returned Span.
 *
 * The terminator is allocated immediately after the requested elements and is not included in the
 * returned Span length.
 *
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param count Number of wchar_t elements in the returned Span.
 * @return Span referencing the requested zero-initialized wchar_t elements.
 */
template<>
Span<wchar_t> SystemPushArrayZero(MemoryArena memoryArena, size_t count);

/**
 * Allocates storage for one structure from a MemoryArena.
 *
 * No constructor is invoked; this is raw arena allocation for T.
 *
 * @tparam T Structure type to allocate.
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @return Pointer to the allocated storage, or nullptr if the arena cannot satisfy the allocation.
 */
template<typename T>
T* SystemPushStruct(MemoryArena memoryArena);

/**
 * Allocates zero-initialized storage for one structure from a MemoryArena.
 *
 * No constructor is invoked; this is raw zeroed arena allocation for T.
 *
 * @tparam T Structure type to allocate.
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @return Pointer to the zero-initialized storage, or nullptr if the arena cannot satisfy the allocation.
 */
template<typename T>
T* SystemPushStructZero(MemoryArena memoryArena);

/**
 * Copies all source elements into an existing destination buffer.
 *
 * The destination must contain at least source.Length elements. No allocation is performed.
 *
 * @tparam T Element type stored in the buffers.
 * @param destination Destination buffer.
 * @param source Source buffer to copy.
 */
template<typename T>
void SystemCopyBuffer(Span<T> destination, ReadOnlySpan<T> source);

/**
 * Allocates a new buffer in a MemoryArena and copies the source elements into it.
 *
 * @tparam T Element type stored in the buffer.
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param source Source buffer to duplicate.
 * @return Span referencing the newly allocated copy.
 */
template<typename T>
Span<T> SystemDuplicateBuffer(MemoryArena memoryArena, ReadOnlySpan<T> source);

/**
 * Allocates a new char buffer in a MemoryArena and copies the source into it.
 *
 * The char specialization preserves zero-initialized storage after the copied data so the result can
 * be used by code that expects a zero-terminated character sequence.
 *
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param source Source character buffer to duplicate.
 * @return Span referencing the newly allocated copy.
 */
template<>
Span<char> SystemDuplicateBuffer(MemoryArena memoryArena, ReadOnlySpan<char> source);

/**
 * Allocates a buffer containing the concatenation of two source buffers.
 *
 * @tparam T Element type stored in the buffers.
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param buffer1 First source buffer.
 * @param buffer2 Second source buffer.
 * @return Span referencing the concatenated buffer.
 */
template<typename T>
Span<T> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2);

/**
 * Allocates a char buffer containing the concatenation of two source buffers.
 *
 * The specialization allocates an additional zero terminator after the returned Span.
 *
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param buffer1 First source character buffer.
 * @param buffer2 Second source character buffer.
 * @return Span referencing the concatenated characters, excluding the trailing terminator.
 */
template<>
Span<char> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<char> buffer1, ReadOnlySpan<char> buffer2);

/**
 * Allocates a wchar_t buffer containing the concatenation of two source buffers.
 *
 * The specialization allocates an additional zero terminator after the returned Span.
 *
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param buffer1 First source wide-character buffer.
 * @param buffer2 Second source wide-character buffer.
 * @return Span referencing the concatenated characters, excluding the trailing terminator.
 */
template<>
Span<wchar_t> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<wchar_t> buffer1, ReadOnlySpan<wchar_t> buffer2);
