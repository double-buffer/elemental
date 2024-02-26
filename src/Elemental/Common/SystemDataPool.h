#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

/**
 * A default full structure used as a fallback for the SystemDataPool template when no full data type is specified.
 */
struct SystemDataPoolDefaultFull
{
};

/**
 * Forward declaration of the SystemDataPoolStorage structure.
 * This structure is meant to internally manage storage specifics of a data pool, but its implementation details are abstracted away from the user.
 */
template<typename T, typename TFull>
struct SystemDataPoolStorage;

/**
 * Represents a data pool with specific item types and optionally, a fuller version of each item.
 * This structure allows for the organization and efficient management of a collection of items of type T,
 * with the option to extend each item with additional data of type TFull.
 * 
 * @tparam T The primary type of items stored in the data pool.
 * @tparam TFull The full data type associated with each item, providing additional information or properties.
 */
template<typename T, typename TFull>
struct SystemDataPool
{
    SystemDataPoolStorage<T, TFull>* Storage; ///< Pointer to the underlying storage mechanism of the data pool.
};

/**
 * Creates and initializes a data pool capable of storing items of type T, with an optional fuller version of each item of type TFull.
 * 
 * @tparam T The primary type of items to be stored in the data pool.
 * @tparam TFull The full data type associated with each item, defaulting to SystemDataPoolDefaultFull when not specified.
 * @param memoryArena The memory arena to use for allocating data pool storage.
 * @param maxItems The maximum number of items that the data pool can hold.
 * @return An instance of SystemDataPool configured to store items of type T and TFull.
 */
template<typename T, typename TFull = SystemDataPoolDefaultFull>
SystemDataPool<T, TFull> SystemCreateDataPool(MemoryArena memoryArena, size_t maxItems);

/**
 * Adds an item of type T to the specified data pool and returns a handle to the newly added item.
 * 
 * @tparam T The type of the item to add to the data pool.
 * @tparam TFull The full data type associated with each item in the pool.
 * @param dataPool The data pool to which the item will be added.
 * @param data The item to add to the data pool.
 * @return A handle to the newly added item within the data pool.
 */
template<typename T, typename TFull>
ElemHandle SystemAddDataPoolItem(SystemDataPool<T, TFull> dataPool, T data);
    
/**
 * Adds or updates the fuller version of an item in the data pool, identified by a given handle.
 * 
 * @tparam T The primary type of items stored in the data pool.
 * @tparam TFull The full data type associated with each item.
 * @param dataPool The data pool containing the item.
 * @param handle The handle identifying the item to be extended with fuller data.
 * @param data The fuller version of the item to add or update in the data pool.
 */
template<typename T, typename TFull>
void SystemAddDataPoolItemFull(SystemDataPool<T, TFull> dataPool, ElemHandle handle, TFull data);

/**
 * Removes an item from the data pool, identified by a given handle.
 * 
 * @tparam T The primary type of items stored in the data pool.
 * @tparam TFull The full data type associated with each item.
 * @param dataPool The data pool from which the item will be removed.
 * @param handle The handle identifying the item to remove.
 */
template<typename T, typename TFull>
void SystemRemoveDataPoolItem(SystemDataPool<T, TFull> dataPool, ElemHandle handle);

/**
 * Retrieves a pointer to an item in the data pool, identified by a given handle.
 * 
 * @tparam T The primary type of items stored in the data pool.
 * @tparam TFull The full data type associated with each item.
 * @param dataPool The data pool containing the item.
 * @param handle The handle identifying the item to retrieve.
 * @return A pointer to the item associated with the given handle, or nullptr if the item does not exist.
 */
template<typename T, typename TFull>
T* SystemGetDataPoolItem(SystemDataPool<T, TFull> dataPool, ElemHandle handle);

/**
 * Retrieves a pointer to the fuller version of an item in the data pool, identified by a given handle.
 * 
 * @tparam T The primary type of items stored in the data pool.
 * @tparam TFull The full data type associated with each item.
 * @param dataPool The data pool containing the item.
 * @param handle The handle identifying the item to retrieve its fuller version.
 * @return A pointer to the fuller version of the item associated with the given handle, or nullptr if the fuller data does not exist.
 */
template<typename T, typename TFull>
TFull* SystemGetDataPoolItemFull(SystemDataPool<T, TFull> dataPool, ElemHandle handle);

/**
 * Counts the number of items in the data pool.
 * 
 * @tparam T Primary type of items in the data pool.
 * @tparam TFull Full data type associated with each item.
 * @param dataPool The data pool whose items are to be counted.
 * @return The total count of items in the data pool.
 */
template<typename T, typename TFull>
size_t SystemGetDataPoolItemCount(SystemDataPool<T, TFull> dataPool);
