#pragma once

#include "SystemSpan.h"

struct MemoryArenaStorage;

enum AllocationState
{
    AllocationState_Committed,
    AllocationState_Reserved
};

struct AllocationInfos
{
    size_t CommittedBytes;
    size_t ReservedBytes;
};

struct MemoryArena
{
    MemoryArenaStorage* Storage;
    uint8_t Level;
};

struct MemoryArenaAllocationInfos
{
    size_t AllocatedBytes;
    size_t CommittedBytes;
    size_t MaximumSizeInBytes;
};

struct StackMemoryArena
{
    MemoryArena Arena;
    size_t StartOffsetInBytes;
    size_t StartExtraOffsetInBytes;

    ~StackMemoryArena();

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
 * @return MemoryArena handle referencing the newly allocated storage, or an empty handle when the
 * platform reservation/header commitment cannot be created.
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
 * @return MemoryArena handle referencing the newly allocated storage, or an empty handle when the
 * requested size cannot be represented or the platform allocation fails.
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
 * allocation is thread-safe; StackMemoryArena allocation is thread-local.
 *
 * A committed allocation can be accessed immediately. A reserved allocation only reserves its
 * range in the arena and must be committed with SystemCommitMemory() before access.
 *
 * When a committed allocation cannot be committed by the platform, nullptr is returned. For a
 * regular shared arena the logical reservation remains consumed because rolling back a concurrent
 * bump allocation would be unsafe after another thread may have reserved a later range.
 *
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param sizeInBytes Number of bytes to allocate.
 * @param state Initial allocation state.
 * @return Pointer to the allocated range, or nullptr if the arena cannot satisfy/commit it.
 */
void* SystemPushMemory(MemoryArena memoryArena, size_t sizeInBytes, AllocationState state = AllocationState_Committed);

/**
 * Commits the pages covering a previously allocated range in a MemoryArena.
 *
 * The range must belong to the specified arena. Commitment is tracked at platform page granularity,
 * so pages shared by multiple logical ranges remain committed while any tracked range still needs
 * them. The operation is thread-safe for regular shared MemoryArena instances.
 *
 * If clearMemory is true, pages newly committed by this operation are cleared before use. Use
 * SystemPushMemoryZero() when the exact returned allocation range must be initialized to zero.
 *
 * A platform failure may occur after earlier pages in the requested range were successfully
 * committed. In that case those successfully committed pages remain tracked and true is not
 * returned; a later call may retry the remaining pages.
 *
 * @param memoryArena MemoryArena containing the range.
 * @param pointer Start of the range to commit.
 * @param sizeInBytes Number of bytes in the range.
 * @param clearMemory Whether newly committed pages should be cleared.
 * @return true when every page covering the requested range is committed; otherwise false.
 */
bool SystemCommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes, bool clearMemory = false);

/**
 * Commits the pages covering a previously allocated buffer in a MemoryArena.
 *
 * @tparam T Element type stored in the buffer.
 * @param memoryArena MemoryArena containing the buffer.
 * @param buffer Buffer whose memory range will be committed.
 * @param clearMemory Whether newly committed pages should be cleared.
 * @return true when every page covering the requested buffer is committed; otherwise false.
 */
template<typename T>
bool SystemCommitMemory(MemoryArena memoryArena, ReadOnlySpan<T> buffer, bool clearMemory = false);

/**
 * Decommits pages that are no longer needed by a range in a MemoryArena.
 *
 * Decommitting memory does not release the logical arena allocation or move the arena pointer. The
 * same reserved range can be committed again later. Physical pages are only decommitted when the
 * arena bookkeeping determines that no remaining committed range still needs that page.
 *
 * The caller is responsible for passing a valid range belonging to the arena and for not accessing
 * the range while it is decommitted. The operation is thread-safe for regular shared MemoryArena
 * instances.
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
 * @return Pointer to the allocated range, or nullptr if the arena cannot satisfy/commit it.
 */
void* SystemPushMemoryZero(MemoryArena memoryArena, size_t sizeInBytes);

/**
 * Allocates a contiguous array from a MemoryArena.
 *
 * The returned Span references arena-owned memory and remains valid only for the lifetime of the
 * corresponding arena allocation context.
 */
template<typename T>
Span<T> SystemPushArray(MemoryArena memoryArena, size_t count, AllocationState state = AllocationState_Committed);

/**
 * Allocates a contiguous array and initializes it to zero.
 */
template<typename T>
Span<T> SystemPushArrayZero(MemoryArena memoryArena, size_t count);

/**
 * Allocates a zero-initialized char array with an additional zero terminator after the returned Span.
 */
template<>
Span<char> SystemPushArrayZero(MemoryArena memoryArena, size_t count);

/**
 * Allocates a zero-initialized wchar_t array with an additional zero terminator after the returned Span.
 */
template<>
Span<wchar_t> SystemPushArrayZero(MemoryArena memoryArena, size_t count);

template<typename T>
T* SystemPushStruct(MemoryArena memoryArena);

template<typename T>
T* SystemPushStructZero(MemoryArena memoryArena);

template<typename T>
void SystemCopyBuffer(Span<T> destination, ReadOnlySpan<T> source);

template<typename T>
Span<T> SystemDuplicateBuffer(MemoryArena memoryArena, ReadOnlySpan<T> source);

template<>
Span<char> SystemDuplicateBuffer(MemoryArena memoryArena, ReadOnlySpan<char> source);

template<typename T>
Span<T> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2);

template<>
Span<char> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<char> buffer1, ReadOnlySpan<char> buffer2);

template<>
Span<wchar_t> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<wchar_t> buffer1, ReadOnlySpan<wchar_t> buffer2);
