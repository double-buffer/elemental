#pragma once

#include "../Elemental/Elemental.h"
#include "SystemMemory.h"

/**
 * Represents a packed data-pool handle decomposed into index and version.
 */
struct SystemDataPoolHandle
{
    uint32_t Index;
    uint32_t Version;
};

/**
 * Default full-data type used when a data pool has no secondary data payload.
 */
struct SystemDataPoolDefaultFull
{
};

template<typename T, typename TFull>
struct SystemDataPoolStorage;

/**
 * Lightweight handle to a fixed-capacity data pool.
 *
 * Data-pool add, remove, lookup, and count operations are thread-safe for a pool created from a
 * shared MemoryArena. Index allocation and recycling are synchronized internally, while lookups
 * validate the item generation without taking the allocation lock.
 *
 * A pointer returned by SystemGetDataPoolItem() or SystemGetDataPoolItemFull() does not pin the
 * item. The caller must guarantee that the same item is not removed or reused while that pointer
 * is being dereferenced. StackMemoryArena-backed pools remain subject to StackMemoryArena's
 * thread-local contract.
 *
 * @tparam T Primary item type.
 * @tparam TFull Optional secondary item type.
 */
template<typename T, typename TFull>
struct SystemDataPool
{
    SystemDataPoolStorage<T, TFull>* Storage;
};

/**
 * Unpacks a data-pool handle into its index and generation.
 *
 * @param packedValue Packed handle value.
 * @return Unpacked index and generation.
 */
SystemDataPoolHandle UnpackSystemDataPoolHandle(uint64_t packedValue);

/**
 * Creates a fixed-capacity data pool.
 *
 * The storage is allocated from memoryArena and is not individually freed. maxItems must fit in the
 * 32-bit handle index space. The returned pool is empty when its backing storage cannot be created.
 *
 * @tparam T Primary item type.
 * @tparam TFull Optional secondary item type.
 * @param memoryArena Arena that owns the pool storage.
 * @param maxItems Maximum number of simultaneously allocated items.
 * @return Data pool backed by memoryArena, or an empty pool on allocation failure.
 */
template<typename T, typename TFull = SystemDataPoolDefaultFull>
SystemDataPool<T, TFull> SystemCreateDataPool(MemoryArena memoryArena, size_t maxItems);

/**
 * Adds an item to the pool.
 *
 * The operation is thread-safe. A recycled slot receives the generation established by its previous
 * removal, so stale handles do not resolve to the new item.
 *
 * @return Handle to the added item, or ELEM_HANDLE_NULL when the pool is full.
 */
template<typename T, typename TFull>
ElemHandle SystemAddDataPoolItem(SystemDataPool<T, TFull> dataPool, T data);
    
/**
 * Writes the optional secondary data associated with an existing item.
 *
 * The handle generation is validated before the write. This function does not pin the item after
 * validation; the caller must not remove or reuse the same item concurrently with this write.
 */
template<typename T, typename TFull>
void SystemAddDataPoolItemFull(SystemDataPool<T, TFull> dataPool, ElemHandle handle, TFull data);

/**
 * Removes an item and makes its slot available for reuse.
 *
 * The operation is thread-safe. Concurrent attempts to remove the same generation only free the
 * slot once; later attempts observe the generation change and are ignored.
 */
template<typename T, typename TFull>
void SystemRemoveDataPoolItem(SystemDataPool<T, TFull> dataPool, ElemHandle handle);

/**
 * Resolves a handle to its primary item.
 *
 * The lookup itself is thread-safe and returns nullptr for a stale handle. The returned pointer is
 * non-owning and is not lifetime-protected against a later concurrent removal/reuse of the same item.
 */
template<typename T, typename TFull>
T* SystemGetDataPoolItem(SystemDataPool<T, TFull> dataPool, ElemHandle handle);

/**
 * Resolves a handle to its secondary item data.
 *
 * The lookup itself is thread-safe and returns nullptr for a stale handle. The returned pointer is
 * non-owning and is not lifetime-protected against a later concurrent removal/reuse of the same item.
 */
template<typename T, typename TFull>
TFull* SystemGetDataPoolItemFull(SystemDataPool<T, TFull> dataPool, ElemHandle handle);

/**
 * Returns the current number of live items.
 *
 * The count is read atomically and may change immediately after the function returns when the pool
 * is being modified concurrently.
 */
template<typename T, typename TFull>
size_t SystemGetDataPoolItemCount(SystemDataPool<T, TFull> dataPool);
