#pragma once

#include "SystemSpan.h"

struct MemoryArenaStorage;

/**
 * Defines the initial virtual-memory state of a MemoryArena allocation.
 */
enum AllocationState
{
    AllocationState_Committed, ///< The allocation is committed and can be accessed immediately.
    AllocationState_Reserved   ///< The allocation reserves arena space but must be committed before access.
};

/**
 * Process-wide virtual-memory allocation counters maintained by the platform layer.
 *
 * These values describe virtual memory managed through Foundations platform-memory functions. They
 * are accounting information rather than ownership handles and can change concurrently as other
 * threads reserve, commit, decommit, or release memory.
 */
struct AllocationInfos
{
    size_t CommittedBytes; ///< Number of bytes currently committed through the platform layer.
    size_t ReservedBytes;  ///< Number of bytes currently reserved through the platform layer.
};

/**
 * Lightweight value handle to MemoryArena storage.
 *
 * Copying a MemoryArena copies only the handle; all copies reference the same MemoryArenaStorage.
 * MemoryArena performs no ownership tracking, reference counting, or automatic lifetime management.
 * Releasing the storage through any copied handle invalidates every other handle and every allocation
 * produced from that storage.
 *
 * Level is zero for regular arenas. Handles obtained from StackMemoryArena use Level to carry the
 * stack lifetime that allocations made through that handle must follow.
 */
struct MemoryArena
{
    MemoryArenaStorage* Storage; ///< Shared allocator and virtual-memory state referenced by this handle.
    uint8_t Level;               ///< Stack lifetime level, or zero for a regular MemoryArena.
};

/**
 * Allocation state of a single MemoryArena.
 */
struct MemoryArenaAllocationInfos
{
    size_t AllocatedBytes;      ///< Logical data bytes currently allocated from the arena.
    size_t CommittedBytes;      ///< Physically committed bytes, including the arena's internal header pages.
    size_t MaximumSizeInBytes;  ///< Maximum logical data capacity requested for the arena.
};

/**
 * Scoped thread-local MemoryArena lifetime.
 *
 * Creating a StackMemoryArena enters a nested stack lifetime. Destroying it rolls back allocations
 * made with that lifetime while preserving allocations explicitly made through ancestor MemoryArena
 * handles. The contained MemoryArena is the value intended to be copied and passed to callees.
 *
 * StackMemoryArena itself must not be copied. A MemoryArena obtained from it must not outlive the
 * corresponding stack scope. StackMemoryArena is thread-local and must not be shared across threads.
 */
struct StackMemoryArena
{
    MemoryArena Arena;               ///< Value handle representing this stack lifetime.
    size_t StartOffsetInBytes;        ///< Main stack-storage offset restored when the scope ends.
    size_t StartExtraOffsetInBytes;   ///< Extra-storage offset restored when the scope ends.

    /**
     * Ends the stack lifetime and rolls back allocations owned by this scope.
     */
    ~StackMemoryArena();

    /**
     * Returns the lightweight MemoryArena handle for this stack lifetime.
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
 * corresponding arena allocation context. Element storage is not initialized by this helper.
 *
 * @tparam T Element type to allocate.
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param count Number of elements to allocate.
 * @param state Initial allocation state for the underlying memory.
 * @return Span referencing the allocated array, or an empty Span when the requested size overflows
 * or the arena cannot satisfy/commit the allocation.
 */
template<typename T>
Span<T> SystemPushArray(MemoryArena memoryArena, size_t count, AllocationState state = AllocationState_Committed);

/**
 * Allocates a contiguous array from a MemoryArena and initializes its storage to zero.
 *
 * @tparam T Element type to allocate.
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param count Number of elements to allocate and clear.
 * @return Span referencing the zero-initialized array, or an empty Span when the requested size
 * overflows or the arena cannot satisfy/commit the allocation.
 */
template<typename T>
Span<T> SystemPushArrayZero(MemoryArena memoryArena, size_t count);

/**
 * Allocates a zero-initialized char array with an additional null terminator after the returned Span.
 *
 * The returned Span Length is exactly count and excludes the terminator. The backing allocation
 * contains count + 1 bytes so Pointer can be consumed by APIs expecting a null-terminated string.
 *
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param count Logical number of characters in the returned Span.
 * @return Span referencing count zero-initialized characters, or an empty Span on failure.
 */
template<>
Span<char> SystemPushArrayZero(MemoryArena memoryArena, size_t count);

/**
 * Allocates a zero-initialized wchar_t array with an additional null terminator after the returned Span.
 *
 * The returned Span Length is exactly count and excludes the terminator. The backing allocation
 * contains count + 1 wchar_t elements.
 *
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @param count Logical number of wide characters in the returned Span.
 * @return Span referencing count zero-initialized wide characters, or an empty Span on failure.
 */
template<>
Span<wchar_t> SystemPushArrayZero(MemoryArena memoryArena, size_t count);

/**
 * Allocates storage for one object from a MemoryArena without initializing it.
 *
 * @tparam T Object type to allocate.
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @return Pointer to arena-owned storage for one T, or nullptr when the allocation cannot be
 * satisfied/committed.
 */
template<typename T>
T* SystemPushStruct(MemoryArena memoryArena);

/**
 * Allocates storage for one object from a MemoryArena and initializes its bytes to zero.
 *
 * This is raw zero-initialization of the allocated storage; constructors are not invoked.
 *
 * @tparam T Object type to allocate.
 * @param memoryArena MemoryArena that provides the allocation lifetime.
 * @return Pointer to zero-initialized arena-owned storage for one T, or nullptr on failure.
 */
template<typename T>
T* SystemPushStructZero(MemoryArena memoryArena);

/**
 * Copies all elements from a source buffer into an existing destination buffer.
 *
 * The destination must contain at least source.Length elements. When it is smaller, the function
 * logs an error and does not perform a partial copy.
 *
 * @tparam T Element type of both buffers.
 * @param destination Writable destination buffer.
 * @param source Source buffer to copy.
 */
template<typename T>
void SystemCopyBuffer(Span<T> destination, ReadOnlySpan<T> source);

/**
 * Allocates a new buffer from a MemoryArena and copies the source elements into it.
 *
 * @tparam T Element type of the source and destination buffers.
 * @param memoryArena MemoryArena that provides the duplicated buffer lifetime.
 * @param source Buffer to duplicate.
 * @return Span referencing the copied elements, or an empty Span when allocation fails.
 */
template<typename T>
Span<T> SystemDuplicateBuffer(MemoryArena memoryArena, ReadOnlySpan<T> source);

/**
 * Duplicates a character buffer and appends a null terminator in the backing allocation.
 *
 * The returned Span preserves source.Length exactly; the terminator is stored immediately after
 * the logical Span and is not included in Length.
 *
 * @param memoryArena MemoryArena that provides the duplicated buffer lifetime.
 * @param source Character buffer to duplicate.
 * @return Span containing a copy of source with a trailing null terminator, or an empty Span on
 * allocation failure.
 */
template<>
Span<char> SystemDuplicateBuffer(MemoryArena memoryArena, ReadOnlySpan<char> source);

/**
 * Allocates a new buffer containing buffer1 immediately followed by buffer2.
 *
 * @tparam T Element type of both input buffers.
 * @param memoryArena MemoryArena that provides the concatenated buffer lifetime.
 * @param buffer1 First buffer in the result.
 * @param buffer2 Second buffer in the result.
 * @return Span whose Length is buffer1.Length + buffer2.Length, or an empty Span when the combined
 * length overflows or allocation fails.
 */
template<typename T>
Span<T> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2);

/**
 * Concatenates two character buffers and appends a null terminator in the backing allocation.
 *
 * The returned Span Length is the sum of the two logical input lengths and excludes the terminator.
 *
 * @param memoryArena MemoryArena that provides the concatenated buffer lifetime.
 * @param buffer1 First character buffer in the result.
 * @param buffer2 Second character buffer in the result.
 * @return Null-terminated concatenated character Span, or an empty Span on overflow/allocation
 * failure.
 */
template<>
Span<char> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<char> buffer1, ReadOnlySpan<char> buffer2);

/**
 * Concatenates two wide-character buffers and appends a null terminator in the backing allocation.
 *
 * The returned Span Length is the sum of the two logical input lengths and excludes the terminator.
 *
 * @param memoryArena MemoryArena that provides the concatenated buffer lifetime.
 * @param buffer1 First wide-character buffer in the result.
 * @param buffer2 Second wide-character buffer in the result.
 * @return Null-terminated concatenated wide-character Span, or an empty Span on
 * overflow/allocation failure.
 */
template<>
Span<wchar_t> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<wchar_t> buffer1, ReadOnlySpan<wchar_t> buffer2);
