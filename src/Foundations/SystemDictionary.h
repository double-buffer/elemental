#pragma once

#include "SystemMemory.h"

template<typename TValue>
struct SystemDictionaryStorage;

/**
 * Fixed-capacity hash dictionary backed by a MemoryArena.
 *
 * Dictionary add, remove, lookup, and contains operations are thread-safe for a dictionary created
 * from a shared MemoryArena. Internal bucket traversal and entry recycling are synchronized so an
 * entry is never published before its hash/value are initialized and a removed slot is not reused
 * while another dictionary operation is traversing it.
 *
 * A pointer returned by SystemGetDictionaryValue(), or a reference returned by operator[], does not
 * pin the entry after the lookup operation completes. The caller must guarantee that the same
 * dictionary entry is not removed or reused while such a pointer/reference is being dereferenced.
 * StackMemoryArena-backed dictionaries remain subject to StackMemoryArena's thread-local contract.
 *
 * @tparam TKey Key type.
 * @tparam TValue Value type.
 */
template<typename TKey, typename TValue>
struct SystemDictionary
{
    SystemDictionaryStorage<TValue>* Storage;
    
    /**
     * Returns the value associated with key.
     *
     * When the key is missing, a zero-initialized thread-local fallback value is returned. This
     * operator is lookup-only; assigning through that fallback does not insert a dictionary entry.
     * The returned reference follows the same lifetime rule as SystemGetDictionaryValue().
     */
    TValue& operator[](TKey key);
};

/**
 * Creates a fixed-capacity dictionary in memoryArena.
 *
 * maxItemsCount must fit in the signed 32-bit internal index space. The returned dictionary is empty
 * when its backing storage cannot be allocated.
 */
template<typename TKey, typename TValue>
SystemDictionary<TKey, TValue> SystemCreateDictionary(MemoryArena memoryArena, size_t maxItemsCount);

/**
 * Adds a dictionary entry.
 *
 * The operation is thread-safe. The dictionary stores a 64-bit hash of the key rather than a copy of
 * the key itself.
 */
template<typename TKey, typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<TKey, TValue> dictionary, TKey key, TValue value);

template<typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key, TValue value);

template<typename TValue, typename T>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key, TValue value);

/**
 * Removes the first entry matching the key hash and recycles its storage.
 *
 * The operation is thread-safe with other dictionary operations. Pointers/references previously
 * returned for the removed entry must no longer be used.
 */
template<typename TKey, typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<TKey, TValue> dictionary, TKey key);

template<typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key);

template<typename TValue, typename T>
void SystemRemoveDictionaryEntry(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key);

/**
 * Resolves a key to its value.
 *
 * The lookup operation is thread-safe. The returned pointer is non-owning and does not protect the
 * entry from a later concurrent removal/reuse; callers retaining the pointer must synchronize that
 * entry's lifetime themselves.
 *
 * @return Pointer to the value, or nullptr when the key is not present.
 */
template<typename TKey, typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionary<TKey, TValue> dictionary, TKey key);

template<typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key);

template<typename TValue, typename T>
TValue* SystemGetDictionaryValue(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key);

/**
 * Tests whether a key is present.
 *
 * The operation is thread-safe. The result is a snapshot and may become stale immediately after
 * return when the dictionary is modified concurrently.
 */
template<typename TKey, typename TValue>
bool SystemDictionaryContainsKey(SystemDictionary<TKey, TValue> dictionary, TKey key);

template<typename TValue>
bool SystemDictionaryContainsKey(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key);

template<typename TValue, typename T>
bool SystemDictionaryContainsKey(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key);

/**
 * Logs the current bucket heads for debugging.
 */
template<typename TKey, typename TValue>
void SystemDebugDictionary(SystemDictionary<TKey, TValue> dictionary);
